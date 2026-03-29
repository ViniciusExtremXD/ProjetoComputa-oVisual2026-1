// Interface gráfica em janela única do projeto.
// A classe reúne a área da imagem, o histograma, os dados técnicos
// e os controles usados para equalizar, salvar e encerrar a aplicação.
// Integrantes:
// Rodrigo Rosalles - 10409316
// Vinícius Magno - 10401365
// Natalia Teixeira - 10395853

#ifndef GUI_H
#define GUI_H

#include <string>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "Histogram.h"
#include "ImageProcessor.h"

namespace window_constants {
    constexpr int MAIN_WIDTH = 1280;
    constexpr int MAIN_HEIGHT = 720;
    constexpr int SIDEBAR_WIDTH = 360;
    constexpr int SIDEBAR_HEIGHT = 720;
}

// As constantes globais simplificam o uso do layout nos arquivos de implementação.
constexpr int MAIN_WIDTH = window_constants::MAIN_WIDTH;
constexpr int MAIN_HEIGHT = window_constants::MAIN_HEIGHT;
constexpr int SIDEBAR_WIDTH = window_constants::SIDEBAR_WIDTH;
constexpr int SIDEBAR_HEIGHT = window_constants::SIDEBAR_HEIGHT;

// Botão simples desenhado com primitivas do SDL.
class Button {
public:
    Button(float x = 0, float y = 0, float width = 0, float height = 0,
           const std::string& button_text = "");

    void handleEvent(const SDL_Event& event);
    [[nodiscard]] bool wasClicked();
    void draw(SDL_Renderer* renderer, TTF_Font* font) const;
    void setText(const std::string& new_text);

private:
    // Área clicável, texto e cores de cada estado do botão.
    SDL_FRect rect_;
    std::string text_;
    SDL_Color normal_color_;
    SDL_Color hover_color_;
    SDL_Color pressed_color_;

    // O clique só é consumido depois de pressionar e soltar dentro do mesmo botão.
    bool is_hovered_;
    bool is_pressed_;
    bool click_pending_;

    [[nodiscard]] const SDL_Color& getCurrentColor() const noexcept;
    void renderCenteredText(SDL_Renderer* renderer, TTF_Font* font) const;
};

// Janela principal da aplicação, com imagem e painel lateral integrados.
class GUI {
public:
    explicit GUI(const std::string& image_path);
    ~GUI();

    void run();

    GUI(const GUI&) = delete;
    GUI& operator=(const GUI&) = delete;
    GUI(GUI&&) = delete;
    GUI& operator=(GUI&&) = delete;

private:
    // Recursos criados pelo SDL para a janela única.
    SDL_Window* main_window_;
    SDL_Renderer* main_renderer_;
    SDL_Texture* image_texture_;
    TTF_Font* font_;

    // Estado geral da execução e do arquivo carregado.
    int main_window_width_;
    int main_window_height_;
    bool running_;
    std::string current_image_path_;

    // Controles exibidos na barra lateral.
    Button equalize_button_;
    Button save_button_;
    Button exit_button_;

    // Dados usados na imagem e no painel técnico.
    Histogram histogram_;
    Histogram original_histogram_;
    ImageProcessor image_processor_;

    // Informações de apoio exibidas ao lado do histograma.
    std::string last_save_path_;
    std::string last_save_status_;
    bool image_was_originally_gray_;
    bool is_currently_equalized_;

    void initializeImageAndHistograms(const std::string& image_path);
    void calculateOptimalWindowSize();
    void createWindow();
    void createRenderers();
    void initializeFont();
    void cleanupResources() noexcept;

    void handleEvents();
    void processSystemEvents(const SDL_Event& event);
    void processButtonClicks();
    void handleEqualizeButtonClick();
    void handleSaveButtonClick();
    void handleExitButtonClick();

    void saveCurrentImage();

    void render();
    void renderImageArea();
    void renderSidebar();
    void renderHistogramInformation();
    void renderTechnicalInformation();

    void updateImageTexture();
    void drawText(SDL_Renderer* renderer, const std::string& text,
                  int x, int y, const SDL_Color& color) const;
};

#endif // GUI_H
