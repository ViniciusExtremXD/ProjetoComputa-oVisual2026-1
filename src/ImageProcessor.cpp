/*
 * Arquivo: ImageProcessor.cpp
 * 
 * Descrição:
 * Implementa as rotinas de carregamento, conversão para escala de cinza,
 * equalização de histograma e salvamento das imagens processadas.
 * 
 * Contexto:
 * Centraliza a lógica de transformação da imagem usada pela GUI e pelo
 * modo headless, preservando a imagem base para reversão sem recarregamento.
 * 
 * Autores:
 * Rodrigo Rosalles - 10409316
 * Vinícius Magno - 10401365
 */

#include "../include/ImageProcessor.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include <SDL3_image/SDL_image.h>

namespace {

namespace image_constants {
    constexpr int HISTOGRAM_LEVELS = 256;               // Níveis de histograma (0-255)
    constexpr int JPEG_QUALITY = 95;                    // Qualidade padrão JPEG
    constexpr int GRAYSCALE_COMPONENTS = 3;             // R, G, B para escala de cinza
    
    // Coeficientes exigidos pelo enunciado para conversão RGB -> escala de cinza
    constexpr float LUMINANCE_RED = 0.2125f;
    constexpr float LUMINANCE_GREEN = 0.7154f;
    constexpr float LUMINANCE_BLUE = 0.0721f;
    
    // Formatos de pixel suportados
    constexpr SDL_PixelFormat RGBA_FORMAT = SDL_PIXELFORMAT_RGBA8888;
    constexpr SDL_PixelFormat RGB_FORMAT = SDL_PIXELFORMAT_RGB24;
    
    // Extensões de arquivo suportadas
    constexpr std::string_view PNG_EXTENSION = ".png";
    constexpr std::string_view JPG_EXTENSION = ".jpg";
    constexpr std::string_view JPEG_EXTENSION = ".jpeg";
    constexpr std::string_view BMP_EXTENSION = ".bmp";
    
    // Configurações de debug e logging
    constexpr std::string_view TEMP_DIR = "/tmp";
    constexpr std::string_view TEMP_PREFIX = "processador_output_";
}

class SurfaceLocker {
public:
    explicit SurfaceLocker(SDL_Surface* surface) : surface_(surface), is_locked_(false) {
        if (surface_ && SDL_LockSurface(surface_)) {
            is_locked_ = true;
        }
    }
    
    ~SurfaceLocker() {
        if (is_locked_ && surface_) {
            SDL_UnlockSurface(surface_);
        }
    }
    
    // Impedir cópia e movimento
    SurfaceLocker(const SurfaceLocker&) = delete;
    SurfaceLocker& operator=(const SurfaceLocker&) = delete;
    SurfaceLocker(SurfaceLocker&&) = delete;
    SurfaceLocker& operator=(SurfaceLocker&&) = delete;
    
    [[nodiscard]] bool isLocked() const noexcept { return is_locked_; }
    
private:
    SDL_Surface* surface_;
    bool is_locked_;
};

using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>;

[[nodiscard]] SurfacePtr makeSurfacePtr(SDL_Surface* surface) noexcept {
    return SurfacePtr(surface, SDL_DestroySurface);
}

[[nodiscard]] bool saveSurfaceAsPng(SDL_Surface* surface, const char* file_path) noexcept {
    if (!surface || !file_path) return false;
    
#if defined(SDL_IMAGE_VERSION_ATLEAST) && SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    return IMG_SavePNG(surface, file_path);
#else
    return IMG_SavePNG(surface, file_path) == 0;
#endif
}

[[nodiscard]] bool saveSurfaceAsJpeg(SDL_Surface* surface, const char* file_path, int quality) noexcept {
    if (!surface || !file_path) return false;
    
#if defined(SDL_IMAGE_VERSION_ATLEAST) && SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    return IMG_SaveJPG(surface, file_path, quality);
#else
    return IMG_SaveJPG(surface, file_path, quality) == 0;
#endif
}

[[nodiscard]] bool saveSurfaceAsBmp(SDL_Surface* surface, const char* file_path) noexcept {
    if (!surface || !file_path) return false;
    
#if SDL_VERSION_ATLEAST(3, 0, 0)
    return SDL_SaveBMP(surface, file_path);
#else
    return SDL_SaveBMP(surface, file_path) == 0;
#endif
}

[[nodiscard]] std::string getImageErrorMessage() noexcept {
#if defined(SDL_IMAGE_VERSION_ATLEAST)
#if SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    const char* error_msg = SDL_GetError();
#else
    const char* error_msg = IMG_GetError();
#endif
#elif defined(IMG_GetError)
    const char* error_msg = IMG_GetError();
#else
    const char* error_msg = SDL_GetError();
#endif
    return error_msg ? error_msg : "";
}

[[nodiscard]] constexpr bool isPixelGrayscale(Uint8 red, Uint8 green, Uint8 blue) noexcept {
    return red == green && green == blue;
}

[[nodiscard]] constexpr Uint8 rgbToGrayscale(Uint8 red, Uint8 green, Uint8 blue) noexcept {
    return static_cast<Uint8>(
        image_constants::LUMINANCE_RED * red +
        image_constants::LUMINANCE_GREEN * green +
        image_constants::LUMINANCE_BLUE * blue
    );
}

[[nodiscard]] Uint32 readPixelValue(const Uint8* pixel_data, int bytes_per_pixel) noexcept {
    Uint32 pixel_value = 0;
    std::memcpy(&pixel_value, pixel_data, bytes_per_pixel);
    return pixel_value;
}

void writePixelValue(Uint8* pixel_data, Uint32 pixel_value, int bytes_per_pixel) noexcept {
    std::memcpy(pixel_data, &pixel_value, bytes_per_pixel);
}

[[nodiscard]] SDL_Surface* createGrayscaleSurface(SDL_Surface* source_surface) {
    if (!source_surface) {
        SDL_SetError("Superfície de origem é nula");
        return nullptr;
    }
    
    // Criar superfície de destino
    auto gray_surface = makeSurfacePtr(
        SDL_CreateSurface(source_surface->w, source_surface->h, image_constants::RGBA_FORMAT)
    );
    
    if (!gray_surface) {
        return nullptr;
    }
    
    // Bloquear ambas as superfícies
    SurfaceLocker source_lock(source_surface);
    SurfaceLocker gray_lock(gray_surface.get());
    
    if (!source_lock.isLocked() || !gray_lock.isLocked()) {
        SDL_SetError("Falha ao bloquear superfícies para conversão");
        return nullptr;
    }
    
    // Obter detalhes dos formatos de pixel
    const SDL_PixelFormatDetails* source_format = SDL_GetPixelFormatDetails(source_surface->format);
    const SDL_PixelFormatDetails* gray_format = SDL_GetPixelFormatDetails(gray_surface->format);
    
    if (!source_format || !gray_format) {
        SDL_SetError("Formato de pixel desconhecido para conversão");
        return nullptr;
    }
    
    const Uint8* source_pixels = static_cast<const Uint8*>(source_surface->pixels);
    Uint8* gray_pixels = static_cast<Uint8*>(gray_surface->pixels);
    
    // Processar cada pixel
    for (int y = 0; y < source_surface->h; ++y) {
        const Uint8* source_row = source_pixels + y * source_surface->pitch;
        Uint8* gray_row = gray_pixels + y * gray_surface->pitch;
        
        for (int x = 0; x < source_surface->w; ++x) {
            // Ler pixel da origem
            const Uint32 source_pixel = readPixelValue(
                source_row + x * source_format->bytes_per_pixel,
                source_format->bytes_per_pixel
            );
            
            // Extrair componentes RGBA
            Uint8 red = 0, green = 0, blue = 0, alpha = 0;
            SDL_GetRGBA(source_pixel, source_format, nullptr, &red, &green, &blue, &alpha);
            
            // Converter para escala de cinza
            const Uint8 gray_value = rgbToGrayscale(red, green, blue);
            
            // Mapear pixel em escala de cinza
            const Uint32 gray_pixel = SDL_MapRGBA(
                gray_format, nullptr, gray_value, gray_value, gray_value, alpha
            );
            
            // Escrever pixel no destino
            writePixelValue(
                gray_row + x * gray_format->bytes_per_pixel,
                gray_pixel,
                gray_format->bytes_per_pixel
            );
        }
    }
    
    // Transferir propriedade da superfície
    return gray_surface.release();
}

[[nodiscard]] std::string replaceFileExtension(const std::string& file_path, 
                                               std::string_view new_extension) {
    std::filesystem::path path(file_path);
    path.replace_extension(new_extension);
    return path.string();
}

[[nodiscard]] std::string generateTempPath(std::string_view base_name, 
                                          std::string_view extension) {
    const auto timestamp = std::time(nullptr);
    return std::string(image_constants::TEMP_DIR) + "/" + 
           std::string(image_constants::TEMP_PREFIX) + 
           std::string(base_name) + "_" + 
           std::to_string(timestamp) + 
           std::string(extension);
}

[[nodiscard]] bool testWriteAccess(const std::string& file_path) noexcept {
    std::FILE* test_file = std::fopen(file_path.c_str(), "wb");
    if (!test_file) {
        return false;
    }
    
    // Testar escrita básica
    constexpr unsigned char test_byte = 0x42;
    const bool write_success = (std::fwrite(&test_byte, 1, 1, test_file) == 1);
    
    std::fclose(test_file);
    std::remove(file_path.c_str());  // Limpar arquivo de teste
    
    return write_success;
}

void logSurfaceInfo(SDL_Surface* surface, std::string_view context) noexcept {
    if (!surface) {
        std::cerr << "[DEBUG] " << context << ": superfície nula\n";
        return;
    }
    
    const char* format_name = SDL_GetPixelFormatName(surface->format);
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(surface->format);
    
    std::cerr << "[DEBUG] " << context << ": "
              << "w=" << surface->w << " h=" << surface->h
              << " format=" << (format_name ? format_name : "unknown")
              << " depth=" << (format_details ? format_details->bits_per_pixel : 0)
              << " pitch=" << surface->pitch
              << " pixels=" << static_cast<void*>(surface->pixels) << '\n';
}

} // namespace
// IMPLEMENTAÇÃO DA CLASSE IMAGEPROCESSOR
ImageProcessor::ImageProcessor() 
    : original_image_{nullptr}
    , grayscale_image_{nullptr}
    , current_image_{nullptr}
    , is_equalized_{false} {
}

ImageProcessor::~ImageProcessor() {
    clearAllSurfaces();
}

[[nodiscard]] bool ImageProcessor::loadImage(const char* file_path) {
    if (!file_path) {
        std::cerr << "Erro: caminho de arquivo nulo\n";
        return false;
    }
    
    // Carregar imagem usando SDL_image
    auto loaded_surface = makeSurfacePtr(IMG_Load(file_path));
    if (!loaded_surface) {
        std::cerr << "Erro ao carregar imagem: " << SDL_GetError() << '\n';
        return false;
    }
    
    // Converter para formato RGBA consistente
    auto converted_surface = makeSurfacePtr(
        SDL_ConvertSurface(loaded_surface.get(), image_constants::RGBA_FORMAT)
    );
    
    if (!converted_surface) {
        std::cerr << "Erro ao converter formato da imagem: " << SDL_GetError() << '\n';
        return false;
    }
    
    // Determinar se precisa converter para escala de cinza ou se já está
    SDL_Surface* grayscale_candidate = nullptr;
    if (isImageGrayscale(converted_surface.get())) {
        // Imagem já está em escala de cinza, duplicar
        grayscale_candidate = SDL_DuplicateSurface(converted_surface.get());
    } else {
        // Converter imagem colorida para escala de cinza
        grayscale_candidate = createGrayscaleSurface(converted_surface.get());
    }
    
    if (!grayscale_candidate) {
        std::cerr << "Erro ao gerar imagem em escala de cinza: " << SDL_GetError() << '\n';
        return false;
    }
    
    // Limpar superfícies anteriores e configurar novas
    clearAllSurfaces();
    original_image_ = converted_surface.release();
    grayscale_image_ = grayscale_candidate;
    current_image_ = grayscale_image_;
    is_equalized_ = false;
    
    return true;
}

[[nodiscard]] bool ImageProcessor::isImageGrayscale(SDL_Surface* surface) const {
    if (!surface) return false;
    
    SurfaceLocker surface_lock(surface);
    if (!surface_lock.isLocked()) {
        std::cerr << "Aviso: não foi possível travar superfície para verificação de escala de cinza: "
                  << SDL_GetError() << '\n';
        return false;
    }
    
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(surface->format);
    if (!format_details) return false;
    
    const Uint8* pixel_base = static_cast<const Uint8*>(surface->pixels);
    const int image_width = surface->w;
    const int image_height = surface->h;
    const int bytes_per_pixel = format_details->bytes_per_pixel;
    
    // Verificar todos os pixels evita falso positivo em imagens com poucos
    // pontos coloridos espalhados pela cena.
    const int sample_step = 1;
    
    for (int y = 0; y < image_height; y += sample_step) {
        const Uint8* row_pixels = pixel_base + y * surface->pitch;
        
        for (int x = 0; x < image_width; x += sample_step) {
            const Uint32 pixel_value = readPixelValue(
                row_pixels + x * bytes_per_pixel, bytes_per_pixel
            );
            
            Uint8 red = 0, green = 0, blue = 0, alpha = 0;
            SDL_GetRGBA(pixel_value, format_details, nullptr, &red, &green, &blue, &alpha);
            
            if (!isPixelGrayscale(red, green, blue)) {
                return false;
            }
        }
    }
    
    return true;
}

void ImageProcessor::convertToGrayscale() {
    if (!original_image_) return;
    
    SDL_Surface* new_grayscale = createGrayscaleSurface(original_image_);
    if (!new_grayscale) {
        std::cerr << "Erro ao converter para escala de cinza: " << SDL_GetError() << '\n';
        return;
    }
    
    // Limpar superfícies anteriores se necessário
    if (current_image_ && current_image_ != grayscale_image_ && current_image_ != original_image_) {
        SDL_DestroySurface(current_image_);
    }
    
    if (grayscale_image_) {
        SDL_DestroySurface(grayscale_image_);
    }
    
    // Configurar novas superfícies
    grayscale_image_ = new_grayscale;
    current_image_ = grayscale_image_;
    is_equalized_ = false;
}

void ImageProcessor::equalizeHistogram() {
    if (!grayscale_image_) return;
    
    const int image_width = grayscale_image_->w;
    const int image_height = grayscale_image_->h;
    const int total_pixels = image_width * image_height;
    
    if (total_pixels <= 0) return;
    
    // Calcular histograma da imagem atual
    std::array<int, image_constants::HISTOGRAM_LEVELS> histogram{};
    
    if (!calculateImageHistogram(grayscale_image_, histogram)) {
        std::cerr << "Erro ao calcular histograma para equalização\n";
        return;
    }
    
    // Calcular função de distribuição acumulada (CDF)
    std::array<int, image_constants::HISTOGRAM_LEVELS> cumulative_distribution{};
    cumulative_distribution[0] = histogram[0];
    
    for (int intensity = 1; intensity < image_constants::HISTOGRAM_LEVELS; ++intensity) {
        cumulative_distribution[intensity] = cumulative_distribution[intensity - 1] + histogram[intensity];
    }
    
    int cumulative_distribution_min = 0;
    for (int intensity = 0; intensity < image_constants::HISTOGRAM_LEVELS; ++intensity) {
        if (histogram[intensity] > 0) {
            cumulative_distribution_min = cumulative_distribution[intensity];
            break;
        }
    }

    // Criar mapeamento de equalização com ajuste por cdf_min
    std::array<Uint8, image_constants::HISTOGRAM_LEVELS> intensity_mapping{};
    for (int intensity = 0; intensity < image_constants::HISTOGRAM_LEVELS; ++intensity) {
        const int denominator = total_pixels - cumulative_distribution_min;
        if (denominator <= 0) {
            intensity_mapping[intensity] = static_cast<Uint8>(intensity);
            continue;
        }

        const float normalized_cdf = static_cast<float>(
            cumulative_distribution[intensity] - cumulative_distribution_min
        ) / static_cast<float>(denominator);

        const float mapped_value = std::clamp(
            normalized_cdf * (image_constants::HISTOGRAM_LEVELS - 1),
            0.0f,
            static_cast<float>(image_constants::HISTOGRAM_LEVELS - 1)
        );

        intensity_mapping[intensity] = static_cast<Uint8>(mapped_value + 0.5f);
    }
    
    // Aplicar equalização criando nova superfície
    if (!applyIntensityMapping(intensity_mapping)) {
        std::cerr << "Erro ao aplicar equalização de histograma\n";
        return;
    }
    
    is_equalized_ = true;
}

void ImageProcessor::restoreOriginal() {
    if (current_image_ && current_image_ != grayscale_image_) {
        SDL_DestroySurface(current_image_);
    }
    
    current_image_ = grayscale_image_;
    is_equalized_ = false;
}

[[nodiscard]] bool ImageProcessor::saveImage(const char* file_path) const {
    if (!current_image_) {
        std::cerr << "Erro: nenhuma imagem para salvar\n";
        return false;
    }
    
    if (!file_path) {
        std::cerr << "Erro: caminho de arquivo nulo\n";
        return false;
    }
    
    const std::string path_string(file_path);
    
    // Tentar salvar em formatos diferentes com fallbacks
    return tryPngSave(path_string) ||
           tryBmpSave(path_string) ||
           tryJpegSave(path_string) ||
           tryEmergencySave(path_string);
}

[[nodiscard]] int ImageProcessor::getWidth() const noexcept {
    return current_image_ ? current_image_->w : 0;
}

[[nodiscard]] int ImageProcessor::getHeight() const noexcept {
    return current_image_ ? current_image_->h : 0;
}

[[nodiscard]] SDL_Surface* ImageProcessor::getCurrentImage() const noexcept {
    return current_image_;
}

[[nodiscard]] SDL_Surface* ImageProcessor::getOriginalImage() const noexcept {
    return original_image_;
}

[[nodiscard]] SDL_Surface* ImageProcessor::getGrayscaleImage() const noexcept {
    return grayscale_image_;
}

[[nodiscard]] bool ImageProcessor::getIsEqualized() const noexcept {
    return is_equalized_;
}
// MÉTODOS PRIVADOS DE IMPLEMENTAÇÃO
void ImageProcessor::clearAllSurfaces() noexcept {
    // Limpar current_image_ se for diferente das outras
    if (current_image_ && 
        current_image_ != grayscale_image_ && 
        current_image_ != original_image_) {
        SDL_DestroySurface(current_image_);
    }
    current_image_ = nullptr;
    
    // Limpar grayscale_image_
    if (grayscale_image_) {
        SDL_DestroySurface(grayscale_image_);
        grayscale_image_ = nullptr;
    }
    
    // Limpar original_image_
    if (original_image_) {
        SDL_DestroySurface(original_image_);
        original_image_ = nullptr;
    }
}

[[nodiscard]] bool ImageProcessor::calculateImageHistogram(
    SDL_Surface* surface, 
    std::array<int, image_constants::HISTOGRAM_LEVELS>& histogram) const {
    
    if (!surface) return false;
    
    // Inicializar histograma
    histogram.fill(0);
    
    SurfaceLocker surface_lock(surface);
    if (!surface_lock.isLocked()) {
        std::cerr << "Erro ao bloquear superfície para cálculo de histograma: " 
                  << SDL_GetError() << '\n';
        return false;
    }
    
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(surface->format);
    if (!format_details) {
        std::cerr << "Formato de pixel desconhecido para cálculo de histograma\n";
        return false;
    }
    
    const Uint8* pixel_base = static_cast<const Uint8*>(surface->pixels);
    const int bytes_per_pixel = format_details->bytes_per_pixel;
    
    // Processar todos os pixels
    for (int y = 0; y < surface->h; ++y) {
        const Uint8* row_pixels = pixel_base + y * surface->pitch;
        
        for (int x = 0; x < surface->w; ++x) {
            const Uint32 pixel_value = readPixelValue(
                row_pixels + x * bytes_per_pixel, bytes_per_pixel
            );
            
            Uint8 gray_value = 0, green = 0, blue = 0, alpha = 0;
            SDL_GetRGBA(pixel_value, format_details, nullptr, &gray_value, &green, &blue, &alpha);
            
            // Incrementar contador para este nível de intensidade
            histogram[gray_value]++;
        }
    }
    
    return true;
}

[[nodiscard]] bool ImageProcessor::applyIntensityMapping(
    const std::array<Uint8, image_constants::HISTOGRAM_LEVELS>& intensity_mapping) {
    
    if (!grayscale_image_) return false;
    
    // Limpar superfície atual se diferente da escala de cinza
    if (current_image_ && current_image_ != grayscale_image_) {
        SDL_DestroySurface(current_image_);
    }
    
    // Criar nova superfície para resultado
    auto equalized_surface = makeSurfacePtr(
        SDL_CreateSurface(grayscale_image_->w, grayscale_image_->h, image_constants::RGBA_FORMAT)
    );
    
    if (!equalized_surface) {
        current_image_ = grayscale_image_;
        return false;
    }
    
    // Bloquear ambas as superfícies
    SurfaceLocker source_lock(grayscale_image_);
    SurfaceLocker dest_lock(equalized_surface.get());
    
    if (!source_lock.isLocked() || !dest_lock.isLocked()) {
        std::cerr << "Erro ao bloquear superfícies para equalização: " 
                  << SDL_GetError() << '\n';
        current_image_ = grayscale_image_;
        return false;
    }
    
    // Obter detalhes dos formatos
    const SDL_PixelFormatDetails* source_format = SDL_GetPixelFormatDetails(grayscale_image_->format);
    const SDL_PixelFormatDetails* dest_format = SDL_GetPixelFormatDetails(equalized_surface->format);
    
    if (!source_format || !dest_format) {
        std::cerr << "Formato de pixel desconhecido durante equalização\n";
        current_image_ = grayscale_image_;
        return false;
    }
    
    const Uint8* source_pixels = static_cast<const Uint8*>(grayscale_image_->pixels);
    Uint8* dest_pixels = static_cast<Uint8*>(equalized_surface->pixels);
    
    // Aplicar mapeamento a cada pixel
    for (int y = 0; y < grayscale_image_->h; ++y) {
        const Uint8* source_row = source_pixels + y * grayscale_image_->pitch;
        Uint8* dest_row = dest_pixels + y * equalized_surface->pitch;
        
        for (int x = 0; x < grayscale_image_->w; ++x) {
            // Ler pixel original
            const Uint32 source_pixel = readPixelValue(
                source_row + x * source_format->bytes_per_pixel,
                source_format->bytes_per_pixel
            );
            
            Uint8 gray_value = 0, green = 0, blue = 0, alpha = 0;
            SDL_GetRGBA(source_pixel, source_format, nullptr, &gray_value, &green, &blue, &alpha);
            
            // Aplicar mapeamento de equalização
            const Uint8 equalized_value = intensity_mapping[gray_value];
            
            // Criar pixel equalizado
            const Uint32 dest_pixel = SDL_MapRGBA(
                dest_format, nullptr, 
                equalized_value, equalized_value, equalized_value, alpha
            );
            
            // Escrever pixel no destino
            writePixelValue(
                dest_row + x * dest_format->bytes_per_pixel,
                dest_pixel,
                dest_format->bytes_per_pixel
            );
        }
    }
    
    // Configurar nova imagem atual
    current_image_ = equalized_surface.release();
    return true;
}

[[nodiscard]] bool ImageProcessor::tryPngSave(const std::string& file_path) const {
    if (saveSurfaceAsPng(current_image_, file_path.c_str())) {
        std::cout << "Imagem salva com sucesso em: " << file_path << '\n';
        return true;
    }
    
    const std::string error_msg = getImageErrorMessage();
    std::cerr << "Aviso: falha ao salvar PNG: " 
              << (error_msg.empty() ? "erro desconhecido" : error_msg) << '\n';
    return false;
}

[[nodiscard]] bool ImageProcessor::tryBmpSave(const std::string& file_path) const {
    const std::string bmp_path = replaceFileExtension(file_path, image_constants::BMP_EXTENSION);
    
    // Log informações da superfície para debug
    logSurfaceInfo(current_image_, "BMP save attempt");
    
    // Testar acesso de escrita
    if (!testWriteAccess(bmp_path)) {
        std::cerr << "[DEBUG] Não foi possível abrir caminho de saída para escrita: " 
                  << bmp_path << " (" << std::strerror(errno) << ")\n";
    }
    
    // Converter para formato compatível com BMP (RGB24 sem alpha)
    auto save_surface = makeSurfacePtr(
        SDL_ConvertSurface(current_image_, image_constants::RGB_FORMAT)
    );
    
    if (!save_surface) {
        std::cerr << "Aviso: falha ao converter superfície para formato de salvamento: " 
                  << SDL_GetError() << '\n';
        return false;
    }
    
    if (saveSurfaceAsBmp(save_surface.get(), bmp_path.c_str())) {
        std::cout << "Imagem salva com sucesso em BMP: " << bmp_path << '\n';
        return true;
    }
    
    const std::string error_msg = SDL_GetError();
    std::cerr << "Falha ao salvar BMP: " 
              << (error_msg.empty() ? "erro desconhecido" : error_msg) << '\n';
    return false;
}

[[nodiscard]] bool ImageProcessor::tryJpegSave(const std::string& file_path) const {
    const std::string jpeg_path = replaceFileExtension(file_path, image_constants::JPG_EXTENSION);
    
    if (saveSurfaceAsJpeg(current_image_, jpeg_path.c_str(), image_constants::JPEG_QUALITY)) {
        std::cout << "Imagem salva com sucesso em JPG: " << jpeg_path << '\n';
        return true;
    }
    
    const std::string error_msg = getImageErrorMessage();
    std::cerr << "Falha ao salvar JPG: " 
              << (error_msg.empty() ? "erro desconhecido" : error_msg) << '\n';
    return false;
}

[[nodiscard]] bool ImageProcessor::tryEmergencySave(const std::string& file_path) const {
    // Extrair nome base do arquivo
    const std::filesystem::path original_path(file_path);
    const std::string base_name = original_path.stem().string();
    
    // Tentar BMP em diretório temporário
    const std::string temp_bmp_path = generateTempPath(base_name, image_constants::BMP_EXTENSION);
    
    auto save_surface = makeSurfacePtr(
        SDL_ConvertSurface(current_image_, image_constants::RGB_FORMAT)
    );
    
    if (save_surface && saveSurfaceAsBmp(save_surface.get(), temp_bmp_path.c_str())) {
        std::cout << "Imagem salva com sucesso em BMP (tmp): " << temp_bmp_path << '\n';
        return true;
    }
    
    const std::string error_msg = SDL_GetError();
    std::cerr << "Falha ao salvar BMP em /tmp: " 
              << (error_msg.empty() ? "erro desconhecido" : error_msg) << '\n';
    
    std::cerr << "Erro: todas as tentativas de salvamento falharam\n";
    return false;
}


