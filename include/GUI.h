/**
 * @file GUI.h
 * @brief Declarações da interface gráfica do projeto.
 * 
 * Este cabeçalho reúne as classes responsáveis pelos botões, pelas duas
 * janelas da aplicação e pela interação com o usuário durante o
 * processamento da imagem.
 * 
 * @authors
 *  Rodrigo Rosalles - 10409316
 *  Vinícius Magno - 10401365
 * @date 2025
 */

#ifndef GUI_H
#define GUI_H

#include <memory>
#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "tinyfiledialogs.h"
#include "Histogram.h"
#include "ImageProcessor.h"

/**
 * @brief Constantes para dimensões das janelas
 */
namespace window_constants {
    constexpr int MAIN_WIDTH = 800;
    constexpr int MAIN_HEIGHT = 600;
    constexpr int SECONDARY_WIDTH = 400;
    constexpr int SECONDARY_HEIGHT = 520;
}

// Usar constantes do namespace para compatibilidade
constexpr int MAIN_WIDTH = window_constants::MAIN_WIDTH;
constexpr int MAIN_HEIGHT = window_constants::MAIN_HEIGHT;
constexpr int SECONDARY_WIDTH = window_constants::SECONDARY_WIDTH;
constexpr int SECONDARY_HEIGHT = window_constants::SECONDARY_HEIGHT;

/**
 * @brief Classe para botões interativos da interface
 * 
 * Implementa um botão clicável com estados visuais (normal, hover, pressed)
 * e detecção de eventos de mouse para interação do usuário.
 */
class Button {
public:
    /**
     * @brief Construtor do botão
     * 
     * @param x Posição X do botão
     * @param y Posição Y do botão
     * @param width Largura do botão
     * @param height Altura do botão
     * @param button_text Texto a ser exibido no botão
     */
    Button(float x = 0, float y = 0, float width = 0, float height = 0, 
           const std::string& button_text = "");

    /**
     * @brief Processa eventos SDL para o botão
     * @param event Evento SDL a ser processado
     */
    void handleEvent(const SDL_Event& event);

    /**
     * @brief Verifica se o botão foi clicado
     * @return true se o botão foi clicado neste frame
     */
    [[nodiscard]] bool wasClicked();

    /**
     * @brief Renderiza o botão
     * @param renderer Renderer SDL para desenho
     * @param font Fonte para texto (pode ser nullptr)
     */
    void draw(SDL_Renderer* renderer, TTF_Font* font) const;

    /**
     * @brief Define novo texto para o botão
     * @param new_text Novo texto a ser exibido
     */
    void setText(const std::string& new_text);

private:
    SDL_FRect rect_;                    ///< Retângulo do botão
    std::string text_;                  ///< Texto do botão
    SDL_Color normal_color_;            ///< Cor normal do botão
    SDL_Color hover_color_;             ///< Cor quando mouse está sobre o botão
    SDL_Color pressed_color_;           ///< Cor quando botão está pressionado
    bool is_hovered_;                   ///< Estado de hover
    bool is_pressed_;                   ///< Estado de pressão
    bool click_pending_;                ///< Clique completo (down+up) aguardando consumo

    /**
     * @brief Obtém a cor atual baseada no estado
     * @return Referência para a cor apropriada
     */
    [[nodiscard]] const SDL_Color& getCurrentColor() const noexcept;

    /**
     * @brief Renderiza texto centralizado no botão
     * @param renderer Renderer SDL
     * @param font Fonte para renderização
     */
    void renderCenteredText(SDL_Renderer* renderer, TTF_Font* font) const;
};

/**
 * @brief Classe principal da interface gráfica
 * 
 * Gerencia janelas, eventos, renderização e interação do usuário
 * para processamento interativo de imagens com histogramas.
 */
class GUI {
public:
    /**
     * @brief Construtor da GUI
     * @param image_path Caminho para imagem inicial
     * @throws std::runtime_error se não conseguir inicializar
     */
    explicit GUI(const std::string& image_path);

    /**
     * @brief Destrutor - limpeza automática de recursos
     */
    ~GUI();

    /**
     * @brief Loop principal da interface
     */
    void run();

    // Impedir cópia e movimento (GUI gerencia recursos SDL únicos)
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
    GUI(GUI&&) = delete;
    GUI& operator=(GUI&&) = delete;

private:
    // Recursos SDL
    SDL_Window* main_window_;           ///< Janela principal para imagem
    SDL_Window* secondary_window_;      ///< Janela secundária para histograma
    SDL_Renderer* main_renderer_;       ///< Renderer da janela principal
    SDL_Renderer* secondary_renderer_;  ///< Renderer da janela secundária
    SDL_Texture* image_texture_;        ///< Textura da imagem atual
    TTF_Font* font_;                    ///< Fonte para texto

    // Estado da interface
    int main_window_width_;             ///< Largura da janela principal
    int main_window_height_;            ///< Altura da janela principal
    bool running_;                      ///< Flag de execução do loop principal
    std::string current_image_path_;    ///< Caminho da imagem atual

    // Componentes da interface
    Button open_button_;                ///< Botão para abrir arquivo
    Button save_button_;                ///< Botão para salvar arquivo
    Button equalize_button_;            ///< Botão para equalizar histograma

    // Processamento de dados
    Histogram histogram_;               ///< Histograma da imagem atual
    Histogram original_histogram_;      ///< Histograma da imagem original
    ImageProcessor image_processor_;    ///< Processador de imagens

    // Métodos de inicialização
    void initializeImageAndHistograms(const std::string& image_path);
    void calculateOptimalWindowSize();
    void createWindows();
    void createRenderers();
    void initializeFont();
    void cleanupResources() noexcept;

    // Métodos de evento e renderização
    void handleEvents();
    void processSystemEvents(const SDL_Event& event);
    void processButtonClicks();
    void handleOpenButtonClick();
    void handleEqualizeButtonClick();
    void handleSaveButtonClick();

    void render();
    void renderMainWindow();
    void renderSecondaryWindow();
    void renderHistogramInformation();

    // Métodos utilitários
    void updateImageTexture();
    void drawText(SDL_Renderer* renderer, const std::string& text, 
                  int x, int y, const SDL_Color& color) const;
};

#endif // GUI_H
