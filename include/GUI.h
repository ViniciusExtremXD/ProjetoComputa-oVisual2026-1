/*
 * Arquivo: GUI.h
 * 
 * Descrição:
 * Declara as classes da interface gráfica, incluindo botões, janelas e
 * integração entre visualização da imagem e histograma.
 * 
 * Contexto:
 * Define a camada de interação do projeto, conectando eventos de entrada
 * com as operações de processamento implementadas em ImageProcessor.
 * 
 * Autores:
 * Rodrigo Rosalles - 10409316
 * Vinícius Magno - 10401365
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

namespace window_constants {
    constexpr int MAIN_WIDTH = 800;
    constexpr int MAIN_HEIGHT = 600;
    constexpr int SECONDARY_WIDTH = 400;
    constexpr int SECONDARY_HEIGHT = 640;  // Aumentado de 520 para acomodar painel técnico
}

// Usar constantes do namespace para compatibilidade
constexpr int MAIN_WIDTH = window_constants::MAIN_WIDTH;
constexpr int MAIN_HEIGHT = window_constants::MAIN_HEIGHT;
constexpr int SECONDARY_WIDTH = window_constants::SECONDARY_WIDTH;
constexpr int SECONDARY_HEIGHT = window_constants::SECONDARY_HEIGHT;

class Button {
public:
    Button(float x = 0, float y = 0, float width = 0, float height = 0, 
           const std::string& button_text = "");

    void handleEvent(const SDL_Event& event);

    [[nodiscard]] bool wasClicked();

    void draw(SDL_Renderer* renderer, TTF_Font* font) const;

    void setText(const std::string& new_text);

private:
    SDL_FRect rect_;                    // Retângulo do botão
    std::string text_;                  // Texto do botão
    SDL_Color normal_color_;            // Cor normal do botão
    SDL_Color hover_color_;             // Cor quando mouse está sobre o botão
    SDL_Color pressed_color_;           // Cor quando botão está pressionado
    bool is_hovered_;                   // Estado de hover
    bool is_pressed_;                   // Estado de pressão
    bool click_pending_;                // Clique completo (down+up) aguardando consumo

    [[nodiscard]] const SDL_Color& getCurrentColor() const noexcept;

    void renderCenteredText(SDL_Renderer* renderer, TTF_Font* font) const;
};

class GUI {
public:
    explicit GUI(const std::string& image_path);

    ~GUI();

    void run();

    // Impedir cópia e movimento (GUI gerencia recursos SDL únicos)
    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
    GUI(GUI&&) = delete;
    GUI& operator=(GUI&&) = delete;

private:
    // Recursos SDL
    SDL_Window* main_window_;           // Janela principal para imagem
    SDL_Window* secondary_window_;      // Janela secundária para histograma
    SDL_Renderer* main_renderer_;       // Renderer da janela principal
    SDL_Renderer* secondary_renderer_;  // Renderer da janela secundária
    SDL_Texture* image_texture_;        // Textura da imagem atual
    TTF_Font* font_;                    // Fonte para texto

    // Estado da interface
    int main_window_width_;             // Largura da janela principal
    int main_window_height_;            // Altura da janela principal
    bool running_;                      // Flag de execução do loop principal
    std::string current_image_path_;    // Caminho da imagem atual

    // Componentes da interface
    Button open_button_;                // Botão para abrir arquivo
    Button save_button_;                // Botão para salvar arquivo
    Button equalize_button_;            // Botão para equalizar histograma

    // Processamento de dados
    Histogram histogram_;               // Histograma da imagem atual
    Histogram original_histogram_;      // Histograma da imagem original
    ImageProcessor image_processor_;    // Processador de imagens

    // Estado técnico para painel de informações
    std::string last_save_path_;        // Caminho do último arquivo salvo
    std::string last_save_status_;      // Status do último salvamento (OK/ERRO)
    bool image_was_originally_gray_;    // Flag indicando se imagem original era cinza
    bool is_currently_equalized_;       // Flag indicando se imagem está equalizada

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

    // Método central de salvamento
    void saveCurrentImage();

    void render();
    void renderMainWindow();
    void renderSecondaryWindow();
    void renderHistogramInformation();
    void renderTechnicalInformation();  // Nova função para painel técnico

    // Métodos utilitários
    void updateImageTexture();
    void drawText(SDL_Renderer* renderer, const std::string& text, 
                  int x, int y, const SDL_Color& color) const;
};

#endif // GUI_H



