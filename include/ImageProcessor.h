// Pipeline de processamento usado pela interface e pelo modo headless.
// A classe mantém a imagem original, a base em tons de cinza e o estado atual
// para permitir equalização, reversão e salvamento sem recarregar o arquivo.
// Integrantes:
// Rodrigo Rosalles - 10409316
// Vinícius Magno - 10401365
// Natalia Teixeira - 10395853

#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <array>
#include <string>

#include <SDL3/SDL.h>

// Mantém o estado da imagem ao longo de todo o processamento.
class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    ImageProcessor(const ImageProcessor&) = delete;
    ImageProcessor& operator=(const ImageProcessor&) = delete;
    ImageProcessor(ImageProcessor&&) = default;
    ImageProcessor& operator=(ImageProcessor&&) = default;

    [[nodiscard]] bool loadImage(const char* file_path);
    void convertToGrayscale();
    void equalizeHistogram();
    void restoreOriginal();
    [[nodiscard]] bool saveImage(const char* file_path) const;

    [[nodiscard]] int getWidth() const noexcept;
    [[nodiscard]] int getHeight() const noexcept;
    [[nodiscard]] SDL_Surface* getCurrentImage() const noexcept;
    [[nodiscard]] SDL_Surface* getOriginalImage() const noexcept;
    [[nodiscard]] SDL_Surface* getGrayscaleImage() const noexcept;
    [[nodiscard]] bool getIsEqualized() const noexcept;
    [[nodiscard]] bool isOriginalGrayscale() const noexcept;

private:
    // As três superfícies representam a entrada, a base em cinza e a imagem visível.
    SDL_Surface* original_image_;
    SDL_Surface* grayscale_image_;
    SDL_Surface* current_image_;

    // Essas flags controlam o estado lógico exibido pela interface.
    bool is_equalized_;
    bool is_originally_grayscale_;

    void clearAllSurfaces() noexcept;
    [[nodiscard]] bool isImageGrayscale(SDL_Surface* surface) const;
    [[nodiscard]] bool calculateImageHistogram(
        SDL_Surface* surface,
        std::array<int, 256>& histogram
    ) const;
    [[nodiscard]] bool applyIntensityMapping(
        const std::array<Uint8, 256>& intensity_mapping
    );

    // O salvamento tenta preservar a intenção do usuário antes de recorrer a um fallback.
    [[nodiscard]] bool tryPngSave(const std::string& file_path) const;
    [[nodiscard]] bool tryBmpSave(const std::string& file_path) const;
    [[nodiscard]] bool tryJpegSave(const std::string& file_path) const;
    [[nodiscard]] bool tryEmergencySave(const std::string& file_path) const;
};

#endif // IMAGE_PROCESSOR_H
