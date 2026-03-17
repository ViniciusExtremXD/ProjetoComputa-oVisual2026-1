/*
 * Arquivo: Histogram.cpp
 * 
 * Descrição:
 * Implementa o cálculo do histograma, métricas estatísticas e rotinas de
 * renderização e exportação dos dados da imagem em escala de cinza.
 * 
 * Contexto:
 * Fornece os indicadores usados na janela secundária da interface e nas
 * saídas do modo headless para análise formal dos resultados.
 * 
 * Autores:
 * Rodrigo Rosalles - 10409316
 * Vinícius Magno - 10401365
 */

#include "Histogram.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <string_view>

#include <SDL3_image/SDL_image.h>

namespace {

namespace histogram_constants {
    constexpr int INTENSITY_LEVELS = 256;          // Níveis de intensidade (0-255)
    constexpr double EPSILON = 1e-9;               // Tolerância para operações de ponto flutuante
    constexpr float LUMINANCE_RED = 0.2125f;
    constexpr float LUMINANCE_GREEN = 0.7154f;
    constexpr float LUMINANCE_BLUE = 0.0721f;
    
    // Cores padrão para renderização
    constexpr SDL_Color BACKGROUND{40, 40, 40, 255};       // Fundo do gráfico
    constexpr SDL_Color BORDER{200, 200, 200, 255};        // Bordas do gráfico
    constexpr SDL_Color PRIMARY_BARS{100, 150, 200, 255};  // Barras principais
    constexpr SDL_Color PLOT_BACKGROUND{20, 20, 20, 255};  // Fundo da imagem exportada
    constexpr SDL_Color AXIS_COLOR{200, 200, 200, 255};    // Cor dos eixos
    constexpr SDL_Color EXPORT_BARS{90, 170, 255, 255};    // Barras na imagem exportada
    
    // Classificações de intensidade e contraste
    constexpr float DARK_INTENSITY_THRESHOLD = 85.0f;
    constexpr float BRIGHT_INTENSITY_THRESHOLD = 170.0f;
    constexpr float LOW_CONTRAST_THRESHOLD = 30.0f;
    constexpr float HIGH_CONTRAST_THRESHOLD = 60.0f;
    
    // Parâmetros de renderização
    constexpr int DEFAULT_EXPORT_WIDTH = 800;
    constexpr int DEFAULT_EXPORT_HEIGHT = 600;
    constexpr float PADDING_BOTTOM_RATIO = 1.0f / 12.0f;
    constexpr float PADDING_TOP_RATIO = 1.0f / 18.0f;
    constexpr int MIN_PADDING_BOTTOM = 10;
    constexpr int MIN_PADDING_TOP = 6;
}

namespace classifications {
    constexpr std::string_view DARK_INTENSITY = "escura";
    constexpr std::string_view MEDIUM_INTENSITY = "media";
    constexpr std::string_view BRIGHT_INTENSITY = "clara";
    
    constexpr std::string_view LOW_CONTRAST = "baixo";
    constexpr std::string_view MEDIUM_CONTRAST = "medio";
    constexpr std::string_view HIGH_CONTRAST = "alto";
}

[[nodiscard]] bool saveSurfaceAsPng(SDL_Surface* surface, const char* file_path) noexcept {
    if (!surface || !file_path) return false;
    
#if defined(SDL_IMAGE_VERSION_ATLEAST) && SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    return IMG_SavePNG(surface, file_path);
#else
    return IMG_SavePNG(surface, file_path) == 0;
#endif
}

[[nodiscard]] constexpr Uint32 readPixelValue(const Uint8* pixel_base, int pixel_index, 
                                               Uint8 bytes_per_pixel) noexcept {
    Uint32 pixel_value = 0;
    std::memcpy(&pixel_value, pixel_base + pixel_index * bytes_per_pixel, bytes_per_pixel);
    return pixel_value;
}

void clearSurfaceWithColor(SDL_Surface* surface, const SDL_Color& clear_color) noexcept {
    if (!surface) return;
    
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(surface->format);
    if (!format_details) return;
    
    const Uint32 mapped_color = SDL_MapRGBA(
        format_details, nullptr, 
        clear_color.r, clear_color.g, clear_color.b, clear_color.a
    );
    
    Uint8* pixel_base = static_cast<Uint8*>(surface->pixels);
    const int bytes_per_pixel = format_details->bytes_per_pixel;
    
    for (int y = 0; y < surface->h; ++y) {
        Uint8* row_pixels = pixel_base + y * surface->pitch;
        for (int x = 0; x < surface->w; ++x) {
            std::memcpy(row_pixels + x * bytes_per_pixel, &mapped_color, bytes_per_pixel);
        }
    }
}

void renderHistogramBars(SDL_Renderer* renderer, int x, int y, int width, int height,
                        const std::array<int, histogram_constants::INTENSITY_LEVELS>& histogram_data, 
                        int max_value, const SDL_Color& bar_color) noexcept {
    if (!renderer || max_value <= 0 || width <= 0 || height <= 0) return;
    
    const float bar_width = static_cast<float>(width) / histogram_constants::INTENSITY_LEVELS;
    const float height_scale = static_cast<float>(height) / static_cast<float>(max_value);
    
    SDL_SetRenderDrawColor(renderer, bar_color.r, bar_color.g, bar_color.b, bar_color.a);
    
    for (int intensity = 0; intensity < histogram_constants::INTENSITY_LEVELS; ++intensity) {
        const int pixel_count = histogram_data[intensity];
        if (pixel_count <= 0) continue;
        
        const float bar_height = static_cast<float>(pixel_count) * height_scale;
        if (bar_height <= histogram_constants::EPSILON) continue;
        
        const SDL_FRect bar_rect = {
            x + intensity * bar_width,
            y + (height - bar_height),
            bar_width,
            bar_height
        };
        
        SDL_RenderFillRect(renderer, &bar_rect);
    }
}

void setSurfacePixel(SDL_Surface* surface, int x, int y, const SDL_Color& pixel_color,
                    const SDL_PixelFormatDetails* format_details) noexcept {
    if (!surface || !format_details || 
        x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        return;
    }
    
    const Uint32 mapped_pixel = SDL_MapRGBA(
        format_details, nullptr,
        pixel_color.r, pixel_color.g, pixel_color.b, pixel_color.a
    );
    
    Uint8* row_pixels = static_cast<Uint8*>(surface->pixels) + y * surface->pitch;
    std::memcpy(row_pixels + x * format_details->bytes_per_pixel, 
                &mapped_pixel, format_details->bytes_per_pixel);
}

[[nodiscard]] std::pair<float, float> calculateHistogramStatistics(
    const std::array<int, histogram_constants::INTENSITY_LEVELS>& histogram_data,
    int total_pixels) noexcept {
    
    if (total_pixels <= 0) return {0.0f, 0.0f};
    
    // Calcular média ponderada
    double intensity_sum = 0.0;
    for (int intensity = 0; intensity < histogram_constants::INTENSITY_LEVELS; ++intensity) {
        intensity_sum += static_cast<double>(intensity) * static_cast<double>(histogram_data[intensity]);
    }
    const float mean = static_cast<float>(intensity_sum / static_cast<double>(total_pixels));
    
    // Calcular variância e desvio padrão
    double variance = 0.0;
    for (int intensity = 0; intensity < histogram_constants::INTENSITY_LEVELS; ++intensity) {
        const double difference = static_cast<double>(intensity) - static_cast<double>(mean);
        variance += difference * difference * static_cast<double>(histogram_data[intensity]);
    }
    variance /= static_cast<double>(total_pixels);
    const float standard_deviation = static_cast<float>(std::sqrt(variance));
    
    return {mean, standard_deviation};
}

} // namespace
// IMPLEMENTAÇÃO DA CLASSE HISTOGRAM
Histogram::Histogram() 
    : mean_{0.0f}
    , std_deviation_{0.0f}
    , max_value_{0}
    , total_pixels_{0} {
    data_.fill(0);
}

void Histogram::calculate(SDL_Surface* image_surface) {
    // Reinicializar estado
    data_.fill(0);
    mean_ = 0.0f;
    std_deviation_ = 0.0f;
    max_value_ = 0;
    total_pixels_ = 0;
    
    if (!image_surface) return;
    
    // Bloquear superfície para acesso direto aos pixels
    if (!SDL_LockSurface(image_surface)) return;
    
    // RAII para desbloqueio automático da superfície
    const auto surface_unlock = std::unique_ptr<SDL_Surface, decltype(&SDL_UnlockSurface)>(
        image_surface, SDL_UnlockSurface
    );
    
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(image_surface->format);
    if (!format_details) return;
    
    const int image_width = image_surface->w;
    const int image_height = image_surface->h;
    total_pixels_ = image_width * image_height;
    
    if (total_pixels_ <= 0) return;
    
    const Uint8* pixel_base = static_cast<const Uint8*>(image_surface->pixels);
    const int bytes_per_pixel = format_details->bytes_per_pixel;
    
    // Processar cada pixel da imagem
    for (int y = 0; y < image_height; ++y) {
        const Uint8* row_pixels = pixel_base + y * image_surface->pitch;
        
        for (int x = 0; x < image_width; ++x) {
            const Uint32 pixel_value = readPixelValue(row_pixels, x, bytes_per_pixel);
            
            Uint8 red = 0, green = 0, blue = 0, alpha = 0;
            SDL_GetRGBA(pixel_value, format_details, nullptr, &red, &green, &blue, &alpha);

            const Uint8 gray_value = static_cast<Uint8>(
                histogram_constants::LUMINANCE_RED * red +
                histogram_constants::LUMINANCE_GREEN * green +
                histogram_constants::LUMINANCE_BLUE * blue +
                0.5f
            );
            
            // Incrementar contador para este nível de intensidade
            data_[gray_value]++;
        }
    }
    
    // Calcular valor máximo do histograma
    max_value_ = *std::max_element(data_.begin(), data_.end());
    
    // Calcular estatísticas (média e desvio padrão)
    const auto [calculated_mean, calculated_std_dev] = calculateHistogramStatistics(data_, total_pixels_);
    mean_ = calculated_mean;
    std_deviation_ = calculated_std_dev;
}

void Histogram::draw(SDL_Renderer* renderer, int x, int y, int width, int height) const {
    if (!renderer || max_value_ <= 0 || width <= 0 || height <= 0) return;
    
    // Desenhar fundo do histograma
    SDL_SetRenderDrawColor(renderer, 
                          histogram_constants::BACKGROUND.r, 
                          histogram_constants::BACKGROUND.g,
                          histogram_constants::BACKGROUND.b, 
                          histogram_constants::BACKGROUND.a);
    
    const SDL_FRect background_rect = {
        static_cast<float>(x), static_cast<float>(y), 
        static_cast<float>(width), static_cast<float>(height)
    };
    SDL_RenderFillRect(renderer, &background_rect);
    
    // Renderizar barras do histograma
    renderHistogramBars(renderer, x, y, width, height, data_, max_value_, 
                       histogram_constants::PRIMARY_BARS);
    
    // Desenhar borda
    SDL_SetRenderDrawColor(renderer, 
                          histogram_constants::BORDER.r, 
                          histogram_constants::BORDER.g,
                          histogram_constants::BORDER.b, 
                          histogram_constants::BORDER.a);
    
    const SDL_FRect border_rect = {
        static_cast<float>(x), static_cast<float>(y), 
        static_cast<float>(width), static_cast<float>(height)
    };
    SDL_RenderRect(renderer, &border_rect);
}

void Histogram::drawWithOverlay(SDL_Renderer* renderer, int x, int y, int width, int height,
                               const Histogram& overlay_histogram, 
                               const SDL_Color& overlay_color) const {
    if (!renderer || width <= 0 || height <= 0) return;
    
    // Usar o maior valor entre os dois histogramas para normalização
    const int combined_max_value = std::max(max_value_, overlay_histogram.max_value_);
    if (combined_max_value <= 0) return;
    
    // Desenhar fundo
    SDL_SetRenderDrawColor(renderer, 
                          histogram_constants::BACKGROUND.r, 
                          histogram_constants::BACKGROUND.g,
                          histogram_constants::BACKGROUND.b, 
                          histogram_constants::BACKGROUND.a);
    
    const SDL_FRect background_rect = {
        static_cast<float>(x), static_cast<float>(y), 
        static_cast<float>(width), static_cast<float>(height)
    };
    SDL_RenderFillRect(renderer, &background_rect);
    
    // Renderizar histograma sobreposto primeiro (fundo)
    renderHistogramBars(renderer, x, y, width, height, 
                       overlay_histogram.data_, combined_max_value, overlay_color);
    
    // Renderizar histograma principal por cima
    renderHistogramBars(renderer, x, y, width, height, 
                       data_, combined_max_value, histogram_constants::PRIMARY_BARS);
    
    // Desenhar borda
    SDL_SetRenderDrawColor(renderer, 
                          histogram_constants::BORDER.r, 
                          histogram_constants::BORDER.g,
                          histogram_constants::BORDER.b, 
                          histogram_constants::BORDER.a);
    
    const SDL_FRect border_rect = {
        static_cast<float>(x), static_cast<float>(y), 
        static_cast<float>(width), static_cast<float>(height)
    };
    SDL_RenderRect(renderer, &border_rect);
}

[[nodiscard]] std::string Histogram::getIntensityClassification() const {
    if (mean_ < histogram_constants::DARK_INTENSITY_THRESHOLD) {
        return std::string(classifications::DARK_INTENSITY);
    }
    if (mean_ < histogram_constants::BRIGHT_INTENSITY_THRESHOLD) {
        return std::string(classifications::MEDIUM_INTENSITY);
    }
    return std::string(classifications::BRIGHT_INTENSITY);
}

[[nodiscard]] std::string Histogram::getContrastClassification() const {
    if (std_deviation_ < histogram_constants::LOW_CONTRAST_THRESHOLD) {
        return std::string(classifications::LOW_CONTRAST);
    }
    if (std_deviation_ < histogram_constants::HIGH_CONTRAST_THRESHOLD) {
        return std::string(classifications::MEDIUM_CONTRAST);
    }
    return std::string(classifications::HIGH_CONTRAST);
}

[[nodiscard]] bool Histogram::saveCSV(const std::string& file_path) const {
    if (total_pixels_ <= 0) return false;
    
    std::ofstream csv_file(file_path);
    if (!csv_file.is_open()) return false;
    
    // Escrever cabeçalho CSV
    csv_file << "intensidade,contagem\n";
    
    // Escrever dados do histograma
    for (int intensity = 0; intensity < histogram_constants::INTENSITY_LEVELS; ++intensity) {
        csv_file << intensity << ',' << data_[intensity] << '\n';
    }
    
    return true;
}

[[nodiscard]] bool Histogram::saveSummary(const std::string& file_path) const {
    if (total_pixels_ <= 0) return false;
    
    std::ofstream summary_file(file_path);
    if (!summary_file.is_open()) return false;
    
    // Configurar formatação de ponto flutuante
    summary_file << std::fixed << std::setprecision(2);
    
    // Escrever estatísticas
    summary_file << "Total de pixels: " << total_pixels_ << '\n';
    summary_file << "Média de intensidade: " << mean_ 
                 << " (" << getIntensityClassification() << ")\n";
    summary_file << "Desvio padrão: " << std_deviation_ 
                 << " (" << getContrastClassification() << ")\n";
    
    return true;
}

[[nodiscard]] bool Histogram::savePlotImage(const std::string& file_path, 
                                           int image_width, int image_height) const {
    // Validar parâmetros de entrada
    if (total_pixels_ <= 0 || max_value_ <= 0 || image_width <= 0 || image_height <= 0) {
        return false;
    }
    
    // Usar dimensões padrão se não especificadas
    if (image_width <= 0) image_width = histogram_constants::DEFAULT_EXPORT_WIDTH;
    if (image_height <= 0) image_height = histogram_constants::DEFAULT_EXPORT_HEIGHT;
    
    // Criar superfície para o gráfico
    SDL_Surface* plot_surface = SDL_CreateSurface(image_width, image_height, SDL_PIXELFORMAT_RGBA8888);
    if (!plot_surface) return false;
    
    // RAII para limpeza automática da superfície
    const auto surface_cleanup = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        plot_surface, SDL_DestroySurface
    );
    
    if (!SDL_LockSurface(plot_surface)) return false;
    
    // RAII para desbloqueio automático
    const auto surface_unlock = std::unique_ptr<SDL_Surface, decltype(&SDL_UnlockSurface)>(
        plot_surface, SDL_UnlockSurface
    );
    
    // Limpar superfície com cor de fundo
    clearSurfaceWithColor(plot_surface, histogram_constants::PLOT_BACKGROUND);
    
    // Calcular dimensões da área de plotagem
    const int padding_bottom = std::max(histogram_constants::MIN_PADDING_BOTTOM, 
                                       static_cast<int>(image_height * histogram_constants::PADDING_BOTTOM_RATIO));
    const int padding_top = std::max(histogram_constants::MIN_PADDING_TOP, 
                                    static_cast<int>(image_height * histogram_constants::PADDING_TOP_RATIO));
    const int plot_height = std::max(1, image_height - padding_bottom - padding_top);
    const int axis_y = image_height - padding_bottom;
    
    const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(plot_surface->format);
    if (!format_details) return false;
    
    // Desenhar eixos do gráfico
    // Eixo horizontal (X)
    for (int x = 0; x < image_width; ++x) {
        setSurfacePixel(plot_surface, x, axis_y, histogram_constants::AXIS_COLOR, format_details);
    }
    
    // Eixo vertical (Y)  
    for (int y = padding_top; y < image_height - padding_bottom; ++y) {
        setSurfacePixel(plot_surface, 0, y, histogram_constants::AXIS_COLOR, format_details);
    }
    
    // Calcular escala para as barras
    const double bar_width = static_cast<double>(image_width) / histogram_constants::INTENSITY_LEVELS;
    const double height_scale = static_cast<double>(plot_height) / static_cast<double>(max_value_);
    
    // Desenhar barras do histograma
    for (int intensity = 0; intensity < histogram_constants::INTENSITY_LEVELS; ++intensity) {
        const int pixel_count = data_[intensity];
        if (pixel_count <= 0) continue;
        
        int bar_height = static_cast<int>(std::round(static_cast<double>(pixel_count) * height_scale));
        if (bar_height <= 0) continue;
        if (bar_height > plot_height) bar_height = plot_height;
        
        // Calcular limites da barra
        const int start_x = static_cast<int>(std::floor(intensity * bar_width));
        int end_x = static_cast<int>(std::floor((intensity + 1) * bar_width));
        if (end_x <= start_x) end_x = start_x + 1;
        if (end_x > image_width) end_x = image_width;
        
        // Desenhar barra vertical
        for (int x = start_x; x < end_x; ++x) {
            for (int y = axis_y - 1; y >= axis_y - bar_height && y >= 0; --y) {
                setSurfacePixel(plot_surface, x, y, histogram_constants::EXPORT_BARS, format_details);
            }
        }
    }
    
    // Salvar como PNG
    return saveSurfaceAsPng(plot_surface, file_path.c_str());
}
// MÉTODOS GETTERS INLINE
[[nodiscard]] float Histogram::getMean() const noexcept {
    return mean_;
}

[[nodiscard]] float Histogram::getStdDev() const noexcept {
    return std_deviation_;
}

[[nodiscard]] int Histogram::getMaxValue() const noexcept {
    return max_value_;
}

[[nodiscard]] int Histogram::getTotalPixels() const noexcept {
    return total_pixels_;
}


