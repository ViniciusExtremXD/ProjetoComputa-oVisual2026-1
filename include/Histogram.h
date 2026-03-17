/**
 * @file Histogram.h
 * @brief Declaração da classe usada para analisar e exibir histogramas.
 * 
 * Define a estrutura que calcula o histograma da imagem, extrai as
 * estatísticas principais e também cuida da exportação dos dados e da
 * visualização na interface.
 * 
 * @authors
 *  Rodrigo Rosalles - 10409316
 *  Vinícius Magno - 10401365
 * @date 2025
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <array>
#include <string>

#include <SDL3/SDL.h>

/**
 * @brief Classe para análise estatística e visualização de histogramas
 * 
 * Calcula histogramas de imagens em escala de cinza, fornece estatísticas
 * básicas (média, desvio padrão), classificações automáticas e capacidades
 * de renderização e exportação.
 */
class Histogram {
public:
    /**
     * @brief Construtor padrão - inicializa histograma vazio
     */
    Histogram();

    /**
     * @brief Calcula histograma de uma imagem SDL
     * @param image_surface Superfície SDL da imagem a ser analisada
     */
    void calculate(SDL_Surface* image_surface);

    /**
     * @brief Renderiza histograma simples
     * @param renderer Renderer SDL para desenho
     * @param x Posição X
     * @param y Posição Y
     * @param width Largura da área de desenho
     * @param height Altura da área de desenho
     */
    void draw(SDL_Renderer* renderer, int x, int y, int width, int height) const;

    /**
     * @brief Renderiza histograma com sobreposição comparativa
     * @param renderer Renderer SDL para desenho
     * @param x Posição X
     * @param y Posição Y
     * @param width Largura da área de desenho
     * @param height Altura da área de desenho
     * @param overlay_histogram Histograma a ser sobreposto
     * @param overlay_color Cor do histograma sobreposto
     */
    void drawWithOverlay(SDL_Renderer* renderer, int x, int y, int width, int height,
                        const Histogram& overlay_histogram, 
                        const SDL_Color& overlay_color = {200, 200, 200, 160}) const;

    /**
     * @brief Classifica intensidade da imagem
     * @return String com classificação ("escura", "media", "clara")
     */
    [[nodiscard]] std::string getIntensityClassification() const;

    /**
     * @brief Classifica contraste da imagem
     * @return String com classificação ("baixo", "medio", "alto")
     */
    [[nodiscard]] std::string getContrastClassification() const;

    /**
     * @brief Salva dados do histograma em formato CSV
     * @param file_path Caminho do arquivo CSV
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool saveCSV(const std::string& file_path) const;

    /**
     * @brief Salva resumo estatístico em arquivo texto
     * @param file_path Caminho do arquivo de texto
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool saveSummary(const std::string& file_path) const;

    /**
     * @brief Salva imagem do histograma como PNG
     * @param file_path Caminho do arquivo PNG
     * @param image_width Largura da imagem (padrão: 640)
     * @param image_height Altura da imagem (padrão: 360)
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool savePlotImage(const std::string& file_path, 
                                    int image_width = 640, int image_height = 360) const;

    // Getters para dados estatísticos
    [[nodiscard]] float getMean() const noexcept;
    [[nodiscard]] float getStdDev() const noexcept;
    [[nodiscard]] int getMaxValue() const noexcept;
    [[nodiscard]] int getTotalPixels() const noexcept;

    /**
     * @brief Acesso aos dados brutos do histograma
     * @return Referência constante para array de dados
     */
    [[nodiscard]] const std::array<int, 256>& getData() const noexcept { return data_; }

private:
    std::array<int, 256> data_;         ///< Dados do histograma (256 níveis de intensidade)
    float mean_;                        ///< Média de intensidade calculada
    float std_deviation_;               ///< Desvio padrão calculado
    int max_value_;                     ///< Valor máximo no histograma
    int total_pixels_;                  ///< Total de pixels processados
};

#endif // HISTOGRAM_H
