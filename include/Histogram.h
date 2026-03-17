/*
 * Arquivo: Histogram.h
 * 
 * Descrição:
 * Declara a classe de histograma usada para calcular distribuição de
 * intensidades, estatísticas e rotinas de exportação.
 * 
 * Contexto:
 * Estrutura a interface utilizada pela GUI e pelo modo headless para
 * análise quantitativa da imagem convertida para tons de cinza.
 * 
 * Autores:
 * Rodrigo Rosalles - 10409316
 * Vinícius Magno - 10401365
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <array>
#include <string>

#include <SDL3/SDL.h>

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

    // Getters para dados estatisticos.
    [[nodiscard]] float getMean() const noexcept;
    [[nodiscard]] float getStdDev() const noexcept;
    [[nodiscard]] int getMaxValue() const noexcept;
    [[nodiscard]] int getTotalPixels() const noexcept;

    [[nodiscard]] const std::array<int, 256>& getData() const noexcept { return data_; }

private:
    std::array<int, 256> data_;         // Dados do histograma (256 níveis de intensidade)
    float mean_;                        // Média de intensidade calculada
    float std_deviation_;               // Desvio padrão calculado
    int max_value_;                     // Valor máximo no histograma
    int total_pixels_;                  // Total de pixels processados
};

#endif // HISTOGRAM_H



