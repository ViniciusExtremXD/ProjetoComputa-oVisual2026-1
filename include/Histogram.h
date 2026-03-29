// Estrutura que calcula, classifica e exporta o histograma da imagem.
// Os dados produzidos aqui alimentam a interface e as saídas do modo headless.
// Integrantes:
// Rodrigo Rosalles - 10409316
// Vinícius Magno - 10401365
// Natalia Teixeira - 10395853

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <array>
#include <string>

#include <SDL3/SDL.h>

// Guarda a distribuição de intensidades e os dados estatísticos derivados dela.
class Histogram {
public:
    Histogram();

    void calculate(SDL_Surface* image_surface);
    void draw(SDL_Renderer* renderer, int x, int y, int width, int height) const;
    void drawWithOverlay(SDL_Renderer* renderer, int x, int y, int width, int height,
                         const Histogram& overlay_histogram,
                         const SDL_Color& overlay_color = {200, 200, 200, 160}) const;

    [[nodiscard]] std::string getIntensityClassification() const;
    [[nodiscard]] std::string getContrastClassification() const;
    [[nodiscard]] bool saveCSV(const std::string& file_path) const;
    [[nodiscard]] bool saveSummary(const std::string& file_path) const;
    [[nodiscard]] bool savePlotImage(const std::string& file_path,
                                     int image_width = 640, int image_height = 360) const;

    [[nodiscard]] float getMean() const noexcept;
    [[nodiscard]] float getStdDev() const noexcept;
    [[nodiscard]] int getMaxValue() const noexcept;
    [[nodiscard]] int getTotalPixels() const noexcept;
    [[nodiscard]] const std::array<int, 256>& getData() const noexcept { return data_; }

private:
    // Vetor com 256 intensidades e estatísticas derivadas dele.
    std::array<int, 256> data_;
    float mean_;
    float std_deviation_;
    int max_value_;
    int total_pixels_;
};

#endif // HISTOGRAM_H
