/*
 * Arquivo: ImageProcessor.h
 * 
 * Descrição:
 * Declara a classe responsável por carregar imagens, converter para escala
 * de cinza, equalizar histograma e salvar os resultados.
 * 
 * Contexto:
 * Define a API usada por main e GUI para manter a imagem base em cinza,
 * permitir reversão e garantir consistência no processamento.
 * 
 * Autores:
 * Rodrigo Rosalles - 10409316
 * Vinícius Magno - 10401365
 */

#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <SDL3/SDL.h>
#include <array>
#include <string>

class ImageProcessor {
public:
    ImageProcessor();
    
    ~ImageProcessor();
    
    // Impedir cópia e permitir movimento
    ImageProcessor(const ImageProcessor&) = delete;
    ImageProcessor& operator=(const ImageProcessor&) = delete;
    ImageProcessor(ImageProcessor&&) = default;
    ImageProcessor& operator=(ImageProcessor&&) = default;
    // Métodos principais de processamento.
    [[nodiscard]] bool loadImage(const char* file_path);
    
    void convertToGrayscale();
    
    void equalizeHistogram();
    
    void restoreOriginal();
    
    [[nodiscard]] bool saveImage(const char* file_path) const;
    // Métodos de consulta e acesso.
    [[nodiscard]] int getWidth() const noexcept;
    
    [[nodiscard]] int getHeight() const noexcept;
    
    [[nodiscard]] SDL_Surface* getCurrentImage() const noexcept;
    
    [[nodiscard]] SDL_Surface* getOriginalImage() const noexcept;

    [[nodiscard]] SDL_Surface* getGrayscaleImage() const noexcept;
    
    [[nodiscard]] bool getIsEqualized() const noexcept;

private:
    // Membros privados.
    // Superfície da imagem original (colorida)
    SDL_Surface* original_image_;
    // Superfície da imagem em escala de cinza (base)
    SDL_Surface* grayscale_image_;
    // Superfície da imagem atual (pode ser equalizada ou não)
    SDL_Surface* current_image_;
    // Flag indicando se a imagem atual foi equalizada
    bool is_equalized_;
    // Métodos auxiliares privados.
    void clearAllSurfaces() noexcept;
    
    [[nodiscard]] bool isImageGrayscale(SDL_Surface* surface) const;
    
    [[nodiscard]] bool calculateImageHistogram(
        SDL_Surface* surface,
        std::array<int, 256>& histogram
    ) const;
    
    [[nodiscard]] bool applyIntensityMapping(
        const std::array<Uint8, 256>& intensity_mapping
    );
    // Métodos de salvamento com fallback.
    [[nodiscard]] bool tryPngSave(const std::string& file_path) const;
    
    [[nodiscard]] bool tryBmpSave(const std::string& file_path) const;
    
    [[nodiscard]] bool tryJpegSave(const std::string& file_path) const;
    
    [[nodiscard]] bool tryEmergencySave(const std::string& file_path) const;
};

#endif // IMAGE_PROCESSOR_H



