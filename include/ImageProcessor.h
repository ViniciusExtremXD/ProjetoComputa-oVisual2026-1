/**
 * @file ImageProcessor.h
 * @brief Declarações da classe principal de processamento de imagens.
 * 
 * Aqui ficam as operações centrais do projeto: carregar a imagem,
 * converter para escala de cinza, equalizar o histograma e salvar o
 * resultado final.
 * 
 * @authors
 *  Rodrigo Rosalles - 10409316
 *  Vinícius Magno - 10401365
 * @date 2025
 */

#ifndef IMAGE_PROCESSOR_H
#define IMAGE_PROCESSOR_H

#include <SDL3/SDL.h>
#include <array>
#include <string>

/**
 * @class ImageProcessor
 * @brief Classe responsável pelo processamento e manipulação de imagens
 * 
 * Gerencia carregamento, conversão para escala de cinza, equalização de histograma
 * e salvamento de imagens usando SDL3 e SDL_image. Implementa RAII para
 * gerenciamento automático de recursos.
 */
class ImageProcessor {
public:
    /**
     * @brief Construtor padrão - inicializa processador vazio
     */
    ImageProcessor();
    
    /**
     * @brief Destrutor - limpeza automática de recursos SDL
     */
    ~ImageProcessor();
    
    // Impedir cópia e permitir movimento
    ImageProcessor(const ImageProcessor&) = delete;
    ImageProcessor& operator=(const ImageProcessor&) = delete;
    ImageProcessor(ImageProcessor&&) = default;
    ImageProcessor& operator=(ImageProcessor&&) = default;
    
    // =====================================================
    // MÉTODOS PRINCIPAIS DE PROCESSAMENTO
    // =====================================================
    
    /**
     * @brief Carrega imagem de arquivo e converte para escala de cinza
     * 
     * @param file_path Caminho do arquivo de imagem (PNG, JPG, BMP, etc.)
     * @return true se carregou com sucesso, false caso contrário
     * 
     * @note A imagem é automaticamente convertida para RGBA8888 e depois
     *       para escala de cinza se necessário
     */
    [[nodiscard]] bool loadImage(const char* file_path);
    
    /**
     * @brief Converte imagem atual para escala de cinza
     * 
     * @note Substitui a imagem atual por versão em escala de cinza
     */
    void convertToGrayscale();
    
    /**
     * @brief Aplica equalização de histograma à imagem em escala de cinza
     * 
     * @note Melhora contraste redistribuindo intensidades de forma uniforme
     */
    void equalizeHistogram();
    
    /**
     * @brief Restaura imagem para estado original (antes da equalização)
     */
    void restoreOriginal();
    
    /**
     * @brief Salva imagem atual em arquivo
     * 
     * @param file_path Caminho do arquivo de destino
     * @return true se salvou com sucesso, false caso contrário
     * 
     * @note Tenta múltiplos formatos (PNG, BMP, JPG) como fallback
     */
    [[nodiscard]] bool saveImage(const char* file_path) const;
    
    // =====================================================
    // MÉTODOS DE CONSULTA E ACESSO
    // =====================================================
    
    /**
     * @brief Obtém largura da imagem atual
     * 
     * @return Largura em pixels ou 0 se não há imagem
     */
    [[nodiscard]] int getWidth() const noexcept;
    
    /**
     * @brief Obtém altura da imagem atual
     * 
     * @return Altura em pixels ou 0 se não há imagem
     */
    [[nodiscard]] int getHeight() const noexcept;
    
    /**
     * @brief Obtém ponteiro para superfície da imagem atual
     * 
     * @return Ponteiro para SDL_Surface ou nullptr se não há imagem
     * 
     * @warning O ponteiro retornado é propriedade da classe.
     *          Não deve ser liberado externamente.
     */
    [[nodiscard]] SDL_Surface* getCurrentImage() const noexcept;
    
    /**
     * @brief Obtém ponteiro para superfície da imagem original
     * 
     * @return Ponteiro para SDL_Surface ou nullptr se não há imagem
     * 
     * @warning O ponteiro retornado é propriedade da classe.
     *          Não deve ser liberado externamente.
     */
    [[nodiscard]] SDL_Surface* getOriginalImage() const noexcept;

    /**
     * @brief Obtém ponteiro para a imagem base em escala de cinza
     *
     * @return Ponteiro para SDL_Surface ou nullptr se não há imagem
     */
    [[nodiscard]] SDL_Surface* getGrayscaleImage() const noexcept;
    
    /**
     * @brief Verifica se a imagem atual foi equalizada
     * 
     * @return true se a imagem foi equalizada, false caso contrário
     */
    [[nodiscard]] bool getIsEqualized() const noexcept;

private:
    // =====================================================
    // MEMBROS PRIVADOS
    // =====================================================
    
    /// Superfície da imagem original (colorida)
    SDL_Surface* original_image_;
    
    /// Superfície da imagem em escala de cinza (base)
    SDL_Surface* grayscale_image_;
    
    /// Superfície da imagem atual (pode ser equalizada ou não)
    SDL_Surface* current_image_;
    
    /// Flag indicando se a imagem atual foi equalizada
    bool is_equalized_;
    
    // =====================================================
    // MÉTODOS AUXILIARES PRIVADOS
    // =====================================================
    
    /**
     * @brief Limpa todas as superfícies SDL gerenciadas
     * 
     * @note Chamado pelo destrutor e loadImage para evitar vazamentos
     */
    void clearAllSurfaces() noexcept;
    
    /**
     * @brief Verifica se uma imagem já está em escala de cinza
     * 
     * @param surface Superfície a ser verificada
     * @return true se todos os pixels têm R==G==B
     */
    [[nodiscard]] bool isImageGrayscale(SDL_Surface* surface) const;
    
    /**
     * @brief Calcula histograma de uma imagem em escala de cinza
     * 
     * @param surface Superfície da imagem
     * @param histogram Array de 256 elementos para armazenar resultado
     * @return true se calculou com sucesso
     */
    [[nodiscard]] bool calculateImageHistogram(
        SDL_Surface* surface,
        std::array<int, 256>& histogram
    ) const;
    
    /**
     * @brief Aplica mapeamento de intensidade à imagem
     * 
     * @param intensity_mapping Array de mapeamento 0-255 -> 0-255
     * @return true se aplicou com sucesso
     */
    [[nodiscard]] bool applyIntensityMapping(
        const std::array<Uint8, 256>& intensity_mapping
    );
    
    // =====================================================
    // MÉTODOS DE SALVAMENTO COM FALLBACK
    // =====================================================
    
    /**
     * @brief Tenta salvar como PNG
     * 
     * @param file_path Caminho do arquivo
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool tryPngSave(const std::string& file_path) const;
    
    /**
     * @brief Tenta salvar como BMP
     * 
     * @param file_path Caminho do arquivo
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool tryBmpSave(const std::string& file_path) const;
    
    /**
     * @brief Tenta salvar como JPEG
     * 
     * @param file_path Caminho do arquivo
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool tryJpegSave(const std::string& file_path) const;
    
    /**
     * @brief Tenta salvamento de emergência em diretório temporário
     * 
     * @param file_path Caminho do arquivo original
     * @return true se salvou com sucesso
     */
    [[nodiscard]] bool tryEmergencySave(const std::string& file_path) const;
};

#endif // IMAGE_PROCESSOR_H
