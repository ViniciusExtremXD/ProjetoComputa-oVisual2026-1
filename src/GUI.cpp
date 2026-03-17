/**
 * @file GUI.cpp
 * @brief Implementação da interface gráfica usada no processamento de imagens.
 * 
 * Reúne o comportamento dos botões, a criação das duas janelas e a
 * renderização da imagem com o histograma e os controles da aplicação.
 * 
 * @authors
 *  Rodrigo Rosalles - 10409316
 *  Vinícius Magno - 10401365
 * @date 2025
 */

#include "GUI.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace {

/**
 * @brief Constantes para cores padrão dos botões
 */
namespace button_colors {
    constexpr SDL_Color NORMAL{50, 100, 200, 255};     // Azul padrão
    constexpr SDL_Color HOVER{100, 150, 255, 255};     // Azul claro (hover)
    constexpr SDL_Color PRESSED{30, 60, 150, 255};     // Azul escuro (pressionado)
    constexpr SDL_Color TEXT{255, 255, 255, 255};      // Texto branco
    constexpr SDL_Color BORDER{255, 255, 255, 255};    // Borda branca
}

/**
 * @brief Constantes da interface
 */
namespace ui_constants {
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_DELAY_MS = 1000 / TARGET_FPS;
    constexpr int DEFAULT_FONT_SIZE = 14;
    
    // Cores da interface
    constexpr SDL_Color BACKGROUND_MAIN{0, 0, 0, 255};           // Preto para janela principal
    constexpr SDL_Color BACKGROUND_SECONDARY{30, 30, 30, 255};   // Cinza escuro para histograma
    constexpr SDL_Color TEXT_PRIMARY{255, 255, 255, 255};        // Texto principal
    constexpr SDL_Color TEXT_SECONDARY{200, 200, 200, 255};      // Texto secundário
    constexpr SDL_Color HISTOGRAM_OVERLAY{180, 180, 180, 180};   // Sobreposição do histograma
}

/**
 * @brief Verifica se um ponto está dentro de um retângulo
 * 
 * @param point_x Coordenada X do ponto
 * @param point_y Coordenada Y do ponto
 * @param rect Retângulo para verificação
 * @return true se o ponto está dentro do retângulo
 */
[[nodiscard]] constexpr bool isPointInRect(float point_x, float point_y, const SDL_FRect& rect) noexcept {
    return point_x >= rect.x && point_x <= rect.x + rect.w &&
           point_y >= rect.y && point_y <= rect.y + rect.h;
}

[[nodiscard]] bool isButtonDebugEnabled() noexcept {
    const char* debug_flag = std::getenv("GUI_BUTTON_DEBUG");
    return debug_flag && debug_flag[0] == '1';
}

[[nodiscard]] bool isButtonMouseEvent(const SDL_Event& event) noexcept {
    return event.type == SDL_EVENT_MOUSE_MOTION ||
           event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
           event.type == SDL_EVENT_MOUSE_BUTTON_UP;
}

[[nodiscard]] std::optional<Uint32> getEventWindowId(const SDL_Event& event) noexcept {
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
            return event.motion.windowID;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            return event.button.windowID;
        case SDL_EVENT_MOUSE_WHEEL:
            return event.wheel.windowID;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            return event.key.windowID;
        case SDL_EVENT_WINDOW_SHOWN:
        case SDL_EVENT_WINDOW_HIDDEN:
        case SDL_EVENT_WINDOW_EXPOSED:
        case SDL_EVENT_WINDOW_MOVED:
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_MINIMIZED:
        case SDL_EVENT_WINDOW_MAXIMIZED:
        case SDL_EVENT_WINDOW_RESTORED:
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_HIT_TEST:
        case SDL_EVENT_WINDOW_ICCPROF_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        case SDL_EVENT_WINDOW_SAFE_AREA_CHANGED:
        case SDL_EVENT_WINDOW_OCCLUDED:
        case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
        case SDL_EVENT_WINDOW_DESTROYED:
            return event.window.windowID;
        default:
            return std::nullopt;
    }
}

/**
 * @brief Carrega fonte do sistema com fallback
 * 
 * @param font_size Tamanho da fonte desejado
 * @return Ponteiro para fonte carregada ou nullptr se falhar
 */
[[nodiscard]] TTF_Font* loadSystemFont(int font_size) noexcept {
    constexpr std::array<std::string_view, 6> FONT_PATHS = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };

    for (const std::string_view font_path : FONT_PATHS) {
        if (!std::filesystem::exists(font_path)) {
            continue;
        }

        TTF_Font* font = TTF_OpenFont(std::string(font_path).c_str(), static_cast<float>(font_size));
        if (font) {
            return font;
        }
    }

    return nullptr;
}

} // namespace

// =====================================================
// IMPLEMENTAÇÃO DA CLASSE BUTTON
// =====================================================

/**
 * @brief Construtor do botão com posição, tamanho e texto
 * 
 * @param x Posição X do botão
 * @param y Posição Y do botão  
 * @param width Largura do botão
 * @param height Altura do botão
 * @param button_text Texto a ser exibido no botão
 */
Button::Button(float x, float y, float width, float height, const std::string& button_text)
    : rect_{x, y, width, height}
    , text_{button_text}
    , normal_color_{button_colors::NORMAL}
    , hover_color_{button_colors::HOVER}
    , pressed_color_{button_colors::PRESSED}
    , is_hovered_{false}
    , is_pressed_{false}
    , click_pending_{false} {
}

/**
 * @brief Processa eventos SDL para o botão
 * 
 * @param event Evento SDL a ser processado
 */
void Button::handleEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
            is_hovered_ = isPointInRect(event.motion.x, event.motion.y, rect_);
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (isPointInRect(event.button.x, event.button.y, rect_)) {
                    is_pressed_ = true;
                }
            }
            break;
            
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                const bool released_inside = isPointInRect(event.button.x, event.button.y, rect_);
                if (is_pressed_ && released_inside) {
                    click_pending_ = true;
                }
                is_pressed_ = false;
                is_hovered_ = released_inside;
            }
            break;
            
        default:
            break;
    }

    if (isButtonDebugEnabled() && isButtonMouseEvent(event)) {
        std::cerr << "[GUI_BUTTON_DEBUG] event=" << event.type
                  << " hover=" << is_hovered_
                  << " pressed=" << is_pressed_
                  << " pending=" << click_pending_ << '\n';
    }
}

/**
 * @brief Verifica se o botão foi clicado (pressionado e solto)
 * 
 * @return true se o botão foi clicado neste frame
 */
[[nodiscard]] bool Button::wasClicked() {
    if (click_pending_) {
        click_pending_ = false;
        return true;
    }
    return false;
}

/**
 * @brief Renderiza o botão na tela
 * 
 * @param renderer Renderer SDL para desenho
 * @param font Fonte para renderização do texto (pode ser nullptr)
 */
void Button::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) return;
    
    // Selecionar cor baseada no estado atual
    const SDL_Color& current_color = getCurrentColor();
    
    // Desenhar retângulo preenchido do botão
    SDL_SetRenderDrawColor(renderer, current_color.r, current_color.g, 
                          current_color.b, current_color.a);
    SDL_RenderFillRect(renderer, &rect_);
    
    // Desenhar borda do botão
    SDL_SetRenderDrawColor(renderer, button_colors::BORDER.r, button_colors::BORDER.g, 
                          button_colors::BORDER.b, button_colors::BORDER.a);
    SDL_RenderRect(renderer, &rect_);
    
    // Renderizar texto centralizado se fonte estiver disponível
    if (font && !text_.empty()) {
        renderCenteredText(renderer, font);
    }
}

/**
 * @brief Define novo texto para o botão
 * 
 * @param new_text Novo texto a ser exibido
 */
void Button::setText(const std::string& new_text) {
    text_ = new_text;
}

/**
 * @brief Obtém a cor atual baseada no estado do botão
 * 
 * @return Referência constante para a cor apropriada
 */
[[nodiscard]] const SDL_Color& Button::getCurrentColor() const noexcept {
    if (is_pressed_) {
        return pressed_color_;
    }
    if (is_hovered_) {
        return hover_color_;
    }
    return normal_color_;
}

/**
 * @brief Renderiza texto centralizado no botão
 * 
 * @param renderer Renderer SDL para desenho
 * @param font Fonte para renderização
 */
void Button::renderCenteredText(SDL_Renderer* renderer, TTF_Font* font) const {
    SDL_Surface* text_surface = TTF_RenderText_Blended(
        font, text_.c_str(), text_.length(), button_colors::TEXT
    );
    
    if (!text_surface) return;
    
    // RAII para surface
    const auto surface_cleanup = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        text_surface, SDL_DestroySurface
    );
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) return;
    
    // RAII para texture
    const auto texture_cleanup = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>(
        text_texture, SDL_DestroyTexture
    );
    
    // Calcular posição centralizada do texto
    const SDL_FRect text_rect = {
        rect_.x + (rect_.w - text_surface->w) * 0.5f,
        rect_.y + (rect_.h - text_surface->h) * 0.5f,
        static_cast<float>(text_surface->w),
        static_cast<float>(text_surface->h)
    };
    
    SDL_RenderTexture(renderer, text_texture, nullptr, &text_rect);
}

// =====================================================
// IMPLEMENTAÇÃO DA CLASSE GUI
// =====================================================

/**
 * @brief Construtor da interface gráfica
 * 
 * @param image_path Caminho para a imagem inicial a ser carregada
 * @throws std::runtime_error se não conseguir inicializar a GUI
 */
GUI::GUI(const std::string& image_path) 
    : main_window_{nullptr}
    , secondary_window_{nullptr}
    , main_renderer_{nullptr}
    , secondary_renderer_{nullptr}
    , image_texture_{nullptr}
    , font_{nullptr}
    , main_window_width_{MAIN_WIDTH}
    , main_window_height_{MAIN_HEIGHT}
    , running_{true}
    , current_image_path_{}
    , open_button_{20, 430, 100, 40, "Abrir"}
    , save_button_{280, 430, 100, 40, "Salvar"}
    , equalize_button_{150, 430, 100, 40, "Equalizar"} {
    
    initializeImageAndHistograms(image_path);
    calculateOptimalWindowSize();
    createWindows();
    createRenderers();
    initializeFont();
    updateImageTexture();
}

/**
 * @brief Destrutor da GUI - limpeza automática de recursos
 */
GUI::~GUI() {
    cleanupResources();
}

/**
 * @brief Loop principal da interface gráfica
 * 
 * Executa até que o usuário feche a aplicação
 */
void GUI::run() {
    while (running_) {
        handleEvents();
        render();
        SDL_Delay(ui_constants::FRAME_DELAY_MS);
    }
}

/**
 * @brief Processa eventos SDL da interface
 */
void GUI::handleEvents() {
    SDL_Event event;
    const Uint32 secondary_window_id = secondary_window_ ? SDL_GetWindowID(secondary_window_) : 0;
    
    while (SDL_PollEvent(&event)) {
        // Processar eventos dos botões apenas da janela secundária
        const auto event_window_id = getEventWindowId(event);
        if (event_window_id && *event_window_id == secondary_window_id && isButtonMouseEvent(event)) {
            equalize_button_.handleEvent(event);
            open_button_.handleEvent(event);
            save_button_.handleEvent(event);
        }

        if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE && event_window_id && *event_window_id == secondary_window_id) {
            SDL_Event synthetic_motion{};
            synthetic_motion.type = SDL_EVENT_MOUSE_MOTION;
            synthetic_motion.motion.windowID = secondary_window_id;
            synthetic_motion.motion.x = -1.0f;
            synthetic_motion.motion.y = -1.0f;
            equalize_button_.handleEvent(synthetic_motion);
            open_button_.handleEvent(synthetic_motion);
            save_button_.handleEvent(synthetic_motion);
        }
        
        processSystemEvents(event);
        processButtonClicks();
    }
}

/**
 * @brief Renderiza todas as janelas da interface
 */
void GUI::render() {
    renderMainWindow();
    renderSecondaryWindow();
}

/**
 * @brief Atualiza a textura da imagem atual
 */
void GUI::updateImageTexture() {
    // Limpar textura anterior
    if (image_texture_) {
        SDL_DestroyTexture(image_texture_);
        image_texture_ = nullptr;
    }
    
    // Criar nova textura da imagem atual
    SDL_Surface* current_image = image_processor_.getCurrentImage();
    if (current_image && main_renderer_) {
        image_texture_ = SDL_CreateTextureFromSurface(main_renderer_, current_image);
    }
}

/**
 * @brief Renderiza texto na posição especificada
 * 
 * @param renderer Renderer SDL para desenho
 * @param text Texto a ser renderizado
 * @param x Posição X
 * @param y Posição Y
 * @param color Cor do texto
 */
void GUI::drawText(SDL_Renderer* renderer, const std::string& text, 
                   int x, int y, const SDL_Color& color) const {
    if (!font_ || !renderer || text.empty()) return;
    
    SDL_Surface* text_surface = TTF_RenderText_Blended(
        font_, text.c_str(), text.length(), color
    );
    
    if (!text_surface) return;
    
    // RAII para surface
    const auto surface_cleanup = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        text_surface, SDL_DestroySurface
    );
    
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) return;
    
    // RAII para texture
    const auto texture_cleanup = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>(
        text_texture, SDL_DestroyTexture
    );
    
    const SDL_FRect dest_rect = {
        static_cast<float>(x), 
        static_cast<float>(y),
        static_cast<float>(text_surface->w), 
        static_cast<float>(text_surface->h)
    };
    
    SDL_RenderTexture(renderer, text_texture, nullptr, &dest_rect);
}

// =====================================================
// MÉTODOS PRIVADOS DE INICIALIZAÇÃO
// =====================================================

/**
 * @brief Inicializa imagem e histogramas
 * 
 * @param image_path Caminho da imagem a ser carregada
 * @throws std::runtime_error se não conseguir carregar a imagem
 */
void GUI::initializeImageAndHistograms(const std::string& image_path) {
    if (!image_processor_.loadImage(image_path.c_str())) {
        throw std::runtime_error("Não foi possível carregar a imagem");
    }
    
    // Calcular histogramas inicial (original em escala de cinza e atual)
    original_histogram_.calculate(image_processor_.getGrayscaleImage());
    histogram_.calculate(image_processor_.getCurrentImage());
}

/**
 * @brief Calcula tamanho ótimo da janela baseado na imagem
 */
void GUI::calculateOptimalWindowSize() {
    main_window_width_ = image_processor_.getWidth();
    main_window_height_ = image_processor_.getHeight();
}

/**
 * @brief Cria as janelas principal e secundária
 * 
 * @throws std::runtime_error se não conseguir criar as janelas
 */
void GUI::createWindows() {
    // Obter informações do display para centralização
    const SDL_DisplayMode* display_mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());
    
    int display_width = 800;
    int display_height = 600;
    if (display_mode) {
        display_width = display_mode->w;
        display_height = display_mode->h;
    }
    
    // Calcular posições centralizadas
    const int main_x = (display_width - main_window_width_) / 2;
    const int main_y = (display_height - main_window_height_) / 2;
    
    // Criar janela principal
    main_window_ = SDL_CreateWindow("Processamento de Imagens - Principal",
                                   main_window_width_, main_window_height_, 0);
    if (!main_window_) {
        throw std::runtime_error("Erro ao criar janela principal");
    }
    
    SDL_SetWindowPosition(main_window_, main_x, main_y);
    SDL_SetWindowMinimumSize(main_window_, 320, 240);
    
    // Criar janela secundária posicionada ao lado da principal
    const int secondary_x = main_x + main_window_width_ + 20;
    const int secondary_y = main_y;
    
    secondary_window_ = SDL_CreateWindow("Processamento de Imagens - Histograma",
                                        SECONDARY_WIDTH, SECONDARY_HEIGHT, 0);
    if (!secondary_window_) {
        SDL_DestroyWindow(main_window_);
        throw std::runtime_error("Erro ao criar janela secundária");
    }
    
    // Tentar vincular janela secundária como filha da principal
    if (!SDL_SetWindowParent(secondary_window_, main_window_)) {
        std::cerr << "Aviso: não foi possível vincular a janela secundária: " 
                  << SDL_GetError() << '\n';
    }
    
    SDL_SetWindowPosition(secondary_window_, secondary_x, secondary_y);
}

/**
 * @brief Cria os renderers para as janelas
 * 
 * @throws std::runtime_error se não conseguir criar os renderers
 */
void GUI::createRenderers() {
    main_renderer_ = SDL_CreateRenderer(main_window_, nullptr);
    secondary_renderer_ = SDL_CreateRenderer(secondary_window_, nullptr);
    
    if (!main_renderer_ || !secondary_renderer_) {
        throw std::runtime_error("Erro ao criar renderers");
    }
}

/**
 * @brief Inicializa a fonte TTF
 */
void GUI::initializeFont() {
    if (!TTF_Init()) {
        throw std::runtime_error("TTF não pôde ser inicializado. A GUI requer renderização de texto para o histograma.");
    }
    
    font_ = loadSystemFont(ui_constants::DEFAULT_FONT_SIZE);
    if (!font_) {
        throw std::runtime_error("Não foi possível carregar nenhuma fonte do sistema para a GUI.");
    }
}

/**
 * @brief Limpa todos os recursos da GUI
 */
void GUI::cleanupResources() noexcept {
    if (image_texture_) SDL_DestroyTexture(image_texture_);
    if (font_) TTF_CloseFont(font_);
    if (secondary_renderer_) SDL_DestroyRenderer(secondary_renderer_);
    if (main_renderer_) SDL_DestroyRenderer(main_renderer_);
    if (secondary_window_) SDL_DestroyWindow(secondary_window_);
    if (main_window_) SDL_DestroyWindow(main_window_);
    TTF_Quit();
}

/**
 * @brief Processa eventos do sistema (quit, teclas, etc.)
 * 
 * @param event Evento SDL a ser processado
 */
void GUI::processSystemEvents(const SDL_Event& event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            running_ = false;
            break;
            
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (event.window.windowID == SDL_GetWindowID(main_window_)) {
                running_ = false;
            }
            break;
            
        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_S) {
                bool save_success = image_processor_.saveImage("output_image.png");
                if (!save_success) {
                    std::cerr << "Falha ao salvar imagem de saída\n";
                }
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief Processa cliques dos botões da interface
 */
void GUI::processButtonClicks() {
    if (open_button_.wasClicked()) {
        handleOpenButtonClick();
    }
    
    if (equalize_button_.wasClicked()) {
        handleEqualizeButtonClick();
    }
    
    if (save_button_.wasClicked()) {
        handleSaveButtonClick();
    }
}

/**
 * @brief Processa clique do botão de abrir arquivo
 */
void GUI::handleOpenButtonClick() {
    const char* filter_patterns[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
    const char* filter_description = "Arquivos de imagem";
    
    const char* initial_path = current_image_path_.empty() ? 
        nullptr : current_image_path_.c_str();
    
    char* selected_file = tinyfd_openFileDialog(
        "Abrir Imagem",           // título
        initial_path,             // caminho inicial
        4,                        // número de filtros
        filter_patterns,          // padrões de filtro
        filter_description,       // descrição do filtro
        0                         // múltiplos arquivos (0 = não)
    );
    
    if (selected_file) {
        if (image_processor_.loadImage(selected_file)) {
            current_image_path_ = selected_file;
            original_histogram_.calculate(image_processor_.getGrayscaleImage());
            histogram_.calculate(image_processor_.getCurrentImage());
            updateImageTexture();
            equalize_button_.setText("Equalizar");  // Reset estado
        } else {
            tinyfd_messageBox(
                "Erro",
                "Não foi possível carregar a imagem selecionada.",
                "ok",
                "error",
                1
            );
        }
    }
}

/**
 * @brief Processa clique do botão de equalização
 */
void GUI::handleEqualizeButtonClick() {
    if (image_processor_.getIsEqualized()) {
        image_processor_.restoreOriginal();
        equalize_button_.setText("Equalizar");
    } else {
        image_processor_.equalizeHistogram();
        equalize_button_.setText("Original");
    }
    
    histogram_.calculate(image_processor_.getCurrentImage());
    updateImageTexture();
}

/**
 * @brief Processa clique do botão de salvar arquivo
 */
void GUI::handleSaveButtonClick() {
    const char* filter_patterns[] = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
    const char* filter_description = "Arquivos de imagem";
    
    const char* initial_path = current_image_path_.empty() ? 
        nullptr : current_image_path_.c_str();
    
    char* selected_file = tinyfd_saveFileDialog(
        "Salvar Imagem",          // título
        initial_path,             // caminho inicial
        4,                        // número de filtros
        filter_patterns,          // padrões de filtro
        filter_description        // descrição do filtro
    );
    
    if (selected_file) {
        std::string save_path = selected_file;
        
        // Garantir extensão apropriada
        auto hasExtension = [&save_path](const std::string& ext) {
            return save_path.length() >= ext.length() && 
                   save_path.compare(save_path.length() - ext.length(), ext.length(), ext) == 0;
        };
        
        if (!hasExtension(".png") && !hasExtension(".jpg") && 
            !hasExtension(".jpeg") && !hasExtension(".bmp")) {
            save_path += ".png";  // Extensão padrão
        }
        
        if (!image_processor_.saveImage(save_path.c_str())) {
            tinyfd_messageBox(
                "Erro",
                "Não foi possível salvar a imagem. Verifique as permissões da pasta.",
                "ok",
                "error",
                1
            );
        }
    }
}

/**
 * @brief Renderiza a janela principal com a imagem
 */
void GUI::renderMainWindow() {
    SDL_SetRenderDrawColor(
        main_renderer_, 
        ui_constants::BACKGROUND_MAIN.r, 
        ui_constants::BACKGROUND_MAIN.g,
        ui_constants::BACKGROUND_MAIN.b, 
        ui_constants::BACKGROUND_MAIN.a
    );
    SDL_RenderClear(main_renderer_);
    
    if (image_texture_) {
        SDL_RenderTexture(main_renderer_, image_texture_, nullptr, nullptr);
    }
    
    SDL_RenderPresent(main_renderer_);
}

/**
 * @brief Renderiza a janela secundária com histograma e controles
 */
void GUI::renderSecondaryWindow() {
    SDL_SetRenderDrawColor(
        secondary_renderer_, 
        ui_constants::BACKGROUND_SECONDARY.r, 
        ui_constants::BACKGROUND_SECONDARY.g,
        ui_constants::BACKGROUND_SECONDARY.b, 
        ui_constants::BACKGROUND_SECONDARY.a
    );
    SDL_RenderClear(secondary_renderer_);
    
    // Desenhar histograma com sobreposição do original
    histogram_.drawWithOverlay(
        secondary_renderer_, 
        50, 50, 300, 200, 
        original_histogram_,
        ui_constants::HISTOGRAM_OVERLAY
    );
    
    // Desenhar informações textuais dos histogramas
    renderHistogramInformation();
    
    // Desenhar botões de controle
    open_button_.draw(secondary_renderer_, font_);
    equalize_button_.draw(secondary_renderer_, font_);
    save_button_.draw(secondary_renderer_, font_);
    
    SDL_RenderPresent(secondary_renderer_);
}

/**
 * @brief Renderiza informações textuais dos histogramas
 */
void GUI::renderHistogramInformation() {
    if (!font_) return;
    
    // Informações do histograma atual
    std::stringstream info_stream;
    info_stream << "Atual média: " << static_cast<int>(histogram_.getMean()) 
                << " (" << histogram_.getIntensityClassification() << ")";
    drawText(secondary_renderer_, info_stream.str(), 50, 270, ui_constants::TEXT_PRIMARY);
    
    info_stream.str("");
    info_stream << "Atual desvio: " << static_cast<int>(histogram_.getStdDev()) 
                << " (" << histogram_.getContrastClassification() << ")";
    drawText(secondary_renderer_, info_stream.str(), 50, 300, ui_constants::TEXT_PRIMARY);
    
    // Informações do histograma original
    info_stream.str("");
    info_stream << "Original média: " << static_cast<int>(original_histogram_.getMean())
                << " (" << original_histogram_.getIntensityClassification() << ")";
    drawText(secondary_renderer_, info_stream.str(), 50, 330, ui_constants::TEXT_SECONDARY);
    
    info_stream.str("");
    info_stream << "Original desvio: " << static_cast<int>(original_histogram_.getStdDev())
                << " (" << original_histogram_.getContrastClassification() << ")";
    drawText(secondary_renderer_, info_stream.str(), 50, 360, ui_constants::TEXT_SECONDARY);
    
    // Instruções de uso
    drawText(secondary_renderer_, "Pressione 'S' para salvar", 50, 390, ui_constants::TEXT_SECONDARY);
}
