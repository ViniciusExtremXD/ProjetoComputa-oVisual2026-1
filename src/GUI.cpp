// Implementação da interface gráfica do projeto.
// A janela reúne a imagem processada, o histograma, os dados técnicos
// e os controles usados durante a demonstração do trabalho.
// Integrantes:
// Rodrigo Rosalles - 10409316
// Vinícius Magno - 10401365
// Natalia Teixeira - 10395853

#include "GUI.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace {

namespace button_colors {
    constexpr SDL_Color NORMAL{58, 88, 126, 255};
    constexpr SDL_Color HOVER{74, 108, 150, 255};
    constexpr SDL_Color PRESSED{42, 65, 95, 255};
    constexpr SDL_Color TEXT{245, 247, 250, 255};
    constexpr SDL_Color BORDER{182, 191, 202, 255};
}

namespace ui_constants {
    constexpr int TARGET_FPS = 60;
    constexpr int FRAME_DELAY_MS = 1000 / TARGET_FPS;
    constexpr int DEFAULT_FONT_SIZE = 14;
    constexpr int WINDOW_GAP = 18;
    constexpr int MAIN_IMAGE_PADDING = 18;
    constexpr int WINDOW_EDGE_MARGIN = 24;

    constexpr float PANEL_PADDING = 18.0f;
    constexpr float PANEL_WIDTH = static_cast<float>(SIDEBAR_WIDTH) - PANEL_PADDING * 2.0f;
    constexpr float PANEL_INNER_PADDING = 12.0f;
    constexpr float HISTOGRAM_TOP = 48.0f;
    constexpr float HISTOGRAM_HEIGHT = 170.0f;
    constexpr float STATS_TITLE_TOP = 236.0f;
    constexpr float STATS_TOP = 260.0f;
    constexpr float STATS_HEIGHT = 126.0f;
    constexpr float TECH_TITLE_TOP = 398.0f;
    constexpr float TECH_TOP = 422.0f;
    constexpr float TECH_HEIGHT = 170.0f;
    constexpr float BUTTON_TOP = 610.0f;
    constexpr float BUTTON_HEIGHT = 42.0f;
    constexpr float BUTTON_WIDTH = 96.0f;
    constexpr float BUTTON_GAP = 18.0f;
    constexpr float EQUALIZE_BUTTON_X = PANEL_PADDING;
    constexpr float SAVE_BUTTON_X = EQUALIZE_BUTTON_X + BUTTON_WIDTH + BUTTON_GAP;
    constexpr float EXIT_BUTTON_X = SAVE_BUTTON_X + BUTTON_WIDTH + BUTTON_GAP;

    constexpr SDL_Color BACKGROUND_MAIN{16, 18, 22, 255};
    constexpr SDL_Color BACKGROUND_SECONDARY{26, 29, 34, 255};
    constexpr SDL_Color PANEL_FILL{34, 39, 46, 255};
    constexpr SDL_Color PANEL_BORDER{88, 98, 112, 255};
    constexpr SDL_Color IMAGE_FRAME{72, 80, 92, 255};
    constexpr SDL_Color TITLE_COLOR{236, 240, 245, 255};
    constexpr SDL_Color TEXT_PRIMARY{228, 232, 238, 255};
    constexpr SDL_Color TEXT_SECONDARY{184, 191, 201, 255};
    constexpr SDL_Color TEXT_MUTED{150, 159, 170, 255};
    constexpr SDL_Color HISTOGRAM_OVERLAY{175, 183, 194, 150};
}

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
    // Esse filtro evita que a lógica dos botões reaja a eventos de outra janela do sistema.
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

[[nodiscard]] TTF_Font* loadSystemFont(int font_size) noexcept {
    // A ordem cobre fontes comuns no Linux e deixa um fallback simples para Windows.
    constexpr std::array<std::string_view, 6> font_paths = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "C:\\Windows\\Fonts\\arial.ttf"
    };

    for (const std::string_view font_path : font_paths) {
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

[[nodiscard]] std::string fileNameFromPath(const std::string& path) {
    if (path.empty()) return "(nenhum)";

    const std::filesystem::path fs_path(path);
    const std::string filename = fs_path.filename().string();
    return filename.empty() ? path : filename;
}

[[nodiscard]] std::string detectImageFormatLabel(const std::string& path) {
    const std::string extension = std::filesystem::path(path).extension().string();
    if (extension.empty()) return "desconhecido";

    std::string label = extension;
    if (!label.empty() && label.front() == '.') {
        label.erase(label.begin());
    }

    std::transform(label.begin(), label.end(), label.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return label;
}

[[nodiscard]] std::string currentVideoDriverLabel() {
    const char* driver = SDL_GetCurrentVideoDriver();
    return driver ? driver : "desconhecido";
}

[[nodiscard]] constexpr float sidebarOriginX(int window_width) noexcept {
    return static_cast<float>(window_width - SIDEBAR_WIDTH);
}

[[nodiscard]] constexpr float imageAreaWidth(int window_width) noexcept {
    return static_cast<float>(window_width - SIDEBAR_WIDTH - ui_constants::WINDOW_GAP);
}

void drawPanel(SDL_Renderer* renderer, const SDL_FRect& rect) noexcept {
    if (!renderer) return;

    SDL_SetRenderDrawColor(renderer,
                           ui_constants::PANEL_FILL.r,
                           ui_constants::PANEL_FILL.g,
                           ui_constants::PANEL_FILL.b,
                           ui_constants::PANEL_FILL.a);
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer,
                           ui_constants::PANEL_BORDER.r,
                           ui_constants::PANEL_BORDER.g,
                           ui_constants::PANEL_BORDER.b,
                           ui_constants::PANEL_BORDER.a);
    SDL_RenderRect(renderer, &rect);
}

void drawSeparator(SDL_Renderer* renderer, float x, float y, float width) noexcept {
    if (!renderer || width <= 0.0f) return;

    const SDL_FRect separator_rect{x, y, width, 1.0f};
    SDL_SetRenderDrawColor(renderer,
                           ui_constants::PANEL_BORDER.r,
                           ui_constants::PANEL_BORDER.g,
                           ui_constants::PANEL_BORDER.b,
                           180);
    SDL_RenderFillRect(renderer, &separator_rect);
}

[[nodiscard]] SDL_FRect computeFittedImageRect(const SDL_FRect& viewport,
                                               int image_width,
                                               int image_height) noexcept {
    // A imagem sempre preserva proporção e margem interna dentro do quadro principal.
    const float available_width = viewport.w - 2.0f * ui_constants::MAIN_IMAGE_PADDING;
    const float available_height = viewport.h - 2.0f * ui_constants::MAIN_IMAGE_PADDING;

    if (available_width <= 0.0f || available_height <= 0.0f ||
        image_width <= 0 || image_height <= 0) {
        return {
            viewport.x + static_cast<float>(ui_constants::MAIN_IMAGE_PADDING),
            viewport.y + static_cast<float>(ui_constants::MAIN_IMAGE_PADDING),
            std::max(available_width, 1.0f),
            std::max(available_height, 1.0f)
        };
    }

    const float scale_x = available_width / static_cast<float>(image_width);
    const float scale_y = available_height / static_cast<float>(image_height);
    const float scale = std::min(scale_x, scale_y);

    const float render_width = static_cast<float>(image_width) * scale;
    const float render_height = static_cast<float>(image_height) * scale;

    return {
        std::round(viewport.x + (viewport.w - render_width) * 0.5f),
        std::round(viewport.y + (viewport.h - render_height) * 0.5f),
        std::round(render_width),
        std::round(render_height)
    };
}

} // namespace

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

void Button::handleEvent(const SDL_Event& event) {
    // O clique só é confirmado quando o mouse pressiona e solta dentro do mesmo botão.
    switch (event.type) {
        case SDL_EVENT_MOUSE_MOTION:
            is_hovered_ = isPointInRect(event.motion.x, event.motion.y, rect_);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event.button.button == SDL_BUTTON_LEFT &&
                isPointInRect(event.button.x, event.button.y, rect_)) {
                is_pressed_ = true;
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

[[nodiscard]] bool Button::wasClicked() {
    if (click_pending_) {
        click_pending_ = false;
        return true;
    }
    return false;
}

void Button::draw(SDL_Renderer* renderer, TTF_Font* font) const {
    if (!renderer) return;

    const SDL_Color& current_color = getCurrentColor();

    SDL_SetRenderDrawColor(renderer, current_color.r, current_color.g,
                           current_color.b, current_color.a);
    SDL_RenderFillRect(renderer, &rect_);

    SDL_SetRenderDrawColor(renderer, button_colors::BORDER.r, button_colors::BORDER.g,
                           button_colors::BORDER.b, button_colors::BORDER.a);
    SDL_RenderRect(renderer, &rect_);

    if (font && !text_.empty()) {
        renderCenteredText(renderer, font);
    }
}

void Button::setText(const std::string& new_text) {
    text_ = new_text;
}

[[nodiscard]] const SDL_Color& Button::getCurrentColor() const noexcept {
    if (is_pressed_) {
        return pressed_color_;
    }
    if (is_hovered_) {
        return hover_color_;
    }
    return normal_color_;
}

void Button::renderCenteredText(SDL_Renderer* renderer, TTF_Font* font) const {
    SDL_Surface* text_surface = TTF_RenderText_Blended(
        font, text_.c_str(), text_.length(), button_colors::TEXT
    );

    if (!text_surface) return;

    const auto surface_cleanup = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        text_surface, SDL_DestroySurface
    );

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) return;

    const auto texture_cleanup = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>(
        text_texture, SDL_DestroyTexture
    );

    const SDL_FRect text_rect = {
        rect_.x + (rect_.w - text_surface->w) * 0.5f,
        rect_.y + (rect_.h - text_surface->h) * 0.5f,
        static_cast<float>(text_surface->w),
        static_cast<float>(text_surface->h)
    };

    SDL_RenderTexture(renderer, text_texture, nullptr, &text_rect);
}

GUI::GUI(const std::string& image_path)
    : main_window_{nullptr}
    , main_renderer_{nullptr}
    , image_texture_{nullptr}
    , font_{nullptr}
    , main_window_width_{MAIN_WIDTH}
    , main_window_height_{MAIN_HEIGHT}
    , running_{true}
    , current_image_path_{}
    , equalize_button_{ui_constants::EQUALIZE_BUTTON_X, ui_constants::BUTTON_TOP,
                       ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT, "Equalizar"}
    , save_button_{ui_constants::SAVE_BUTTON_X, ui_constants::BUTTON_TOP,
                   ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT, "Salvar"}
    , exit_button_{ui_constants::EXIT_BUTTON_X, ui_constants::BUTTON_TOP,
                   ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT, "Sair"}
    , last_save_path_{}
    , last_save_status_{"Aguardando"}
    , image_was_originally_gray_{false}
    , is_currently_equalized_{false} {
    // A construção já deixa a interface pronta para entrar no loop principal.
    initializeImageAndHistograms(image_path);
    calculateOptimalWindowSize();
    createWindow();
    createRenderers();
    initializeFont();
    updateImageTexture();
}

GUI::~GUI() {
    cleanupResources();
}

void GUI::run() {
    while (running_) {
        handleEvents();
        render();
        SDL_Delay(ui_constants::FRAME_DELAY_MS);
    }
}

void GUI::handleEvents() {
    SDL_Event event;
    const Uint32 main_window_id = main_window_ ? SDL_GetWindowID(main_window_) : 0;

    while (SDL_PollEvent(&event)) {
        // Os botões só recebem eventos da janela principal para não misturar entrada externa.
        const auto event_window_id = getEventWindowId(event);
        if (event_window_id && *event_window_id == main_window_id && isButtonMouseEvent(event)) {
            equalize_button_.handleEvent(event);
            save_button_.handleEvent(event);
            exit_button_.handleEvent(event);
        }

        if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE &&
            event_window_id && *event_window_id == main_window_id) {
            // Ao sair da janela, um evento sintético limpa hover e pressed sem depender do próximo movimento real.
            SDL_Event synthetic_motion{};
            synthetic_motion.type = SDL_EVENT_MOUSE_MOTION;
            synthetic_motion.motion.windowID = main_window_id;
            synthetic_motion.motion.x = -1.0f;
            synthetic_motion.motion.y = -1.0f;
            equalize_button_.handleEvent(synthetic_motion);
            save_button_.handleEvent(synthetic_motion);
            exit_button_.handleEvent(synthetic_motion);
        }

        processSystemEvents(event);
        processButtonClicks();
    }
}

void GUI::render() {
    // A ordem de desenho é fixa: área da imagem primeiro, painel lateral depois.
    SDL_SetRenderDrawColor(
        main_renderer_,
        ui_constants::BACKGROUND_MAIN.r,
        ui_constants::BACKGROUND_MAIN.g,
        ui_constants::BACKGROUND_MAIN.b,
        ui_constants::BACKGROUND_MAIN.a
    );
    SDL_RenderClear(main_renderer_);

    renderImageArea();
    renderSidebar();
    SDL_RenderPresent(main_renderer_);
}

void GUI::updateImageTexture() {
    // A textura sempre acompanha a superfície atual do processador.
    if (image_texture_) {
        SDL_DestroyTexture(image_texture_);
        image_texture_ = nullptr;
    }

    SDL_Surface* current_image = image_processor_.getCurrentImage();
    if (current_image && main_renderer_) {
        image_texture_ = SDL_CreateTextureFromSurface(main_renderer_, current_image);
    }
}

void GUI::drawText(SDL_Renderer* renderer,
                   const std::string& text,
                   int x,
                   int y,
                   const SDL_Color& color) const {
    if (!font_ || !renderer || text.empty()) return;

    SDL_Surface* text_surface = TTF_RenderText_Blended(
        font_, text.c_str(), text.length(), color
    );

    if (!text_surface) return;

    const auto surface_cleanup = std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>(
        text_surface, SDL_DestroySurface
    );

    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    if (!text_texture) return;

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

void GUI::initializeImageAndHistograms(const std::string& image_path) {
    if (!image_processor_.loadImage(image_path.c_str())) {
        throw std::runtime_error("Não foi possível carregar a imagem");
    }

    // Esse estado inicial alimenta o painel lateral desde o primeiro quadro.
    current_image_path_ = image_path;
    original_histogram_.calculate(image_processor_.getGrayscaleImage());
    histogram_.calculate(image_processor_.getCurrentImage());
    image_was_originally_gray_ = image_processor_.isOriginalGrayscale();
    is_currently_equalized_ = false;
    last_save_path_.clear();
    last_save_status_ = "Aguardando";
}

void GUI::calculateOptimalWindowSize() {
    // O layout foi fixado em 1280x720 para padronizar a apresentação da entrega.
    main_window_width_ = MAIN_WIDTH;
    main_window_height_ = MAIN_HEIGHT;

    const float panel_origin_x = sidebarOriginX(main_window_width_);
    equalize_button_ = Button(panel_origin_x + ui_constants::EQUALIZE_BUTTON_X, ui_constants::BUTTON_TOP,
                              ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT,
                              is_currently_equalized_ ? "Original" : "Equalizar");
    save_button_ = Button(panel_origin_x + ui_constants::SAVE_BUTTON_X, ui_constants::BUTTON_TOP,
                          ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT, "Salvar");
    exit_button_ = Button(panel_origin_x + ui_constants::EXIT_BUTTON_X, ui_constants::BUTTON_TOP,
                          ui_constants::BUTTON_WIDTH, ui_constants::BUTTON_HEIGHT, "Sair");
}

void GUI::createWindow() {
    const SDL_DisplayMode* display_mode = SDL_GetCurrentDisplayMode(SDL_GetPrimaryDisplay());

    int display_width = 800;
    int display_height = 600;
    if (display_mode) {
        display_width = display_mode->w;
        display_height = display_mode->h;
    }

    // A janela nasce centralizada e com tamanho mínimo igual ao layout previsto.
    const int main_x = std::max(
        ui_constants::WINDOW_EDGE_MARGIN,
        (display_width - main_window_width_) / 2
    );
    const int main_y = std::max(
        ui_constants::WINDOW_EDGE_MARGIN,
        (display_height - main_window_height_) / 2
    );

    main_window_ = SDL_CreateWindow("Processamento de Imagens",
                                    main_window_width_, main_window_height_, 0);
    if (!main_window_) {
        throw std::runtime_error("Erro ao criar janela principal");
    }

    SDL_SetWindowPosition(main_window_, main_x, main_y);
    SDL_SetWindowMinimumSize(main_window_, MAIN_WIDTH, MAIN_HEIGHT);
}

void GUI::createRenderers() {
    main_renderer_ = SDL_CreateRenderer(main_window_, nullptr);

    if (!main_renderer_) {
        throw std::runtime_error("Erro ao criar renderers");
    }
}

void GUI::initializeFont() {
    // Sem fonte legível o painel perde utilidade, então a falha aqui interrompe a GUI.
    if (!TTF_Init()) {
        throw std::runtime_error("TTF não pôde ser inicializado. A GUI requer renderização de texto para o histograma.");
    }

    font_ = loadSystemFont(ui_constants::DEFAULT_FONT_SIZE);
    if (!font_) {
        throw std::runtime_error("Não foi possível carregar nenhuma fonte do sistema para a GUI.");
    }
}

void GUI::cleanupResources() noexcept {
    if (image_texture_) SDL_DestroyTexture(image_texture_);
    if (font_) TTF_CloseFont(font_);
    if (main_renderer_) SDL_DestroyRenderer(main_renderer_);
    if (main_window_) SDL_DestroyWindow(main_window_);
    TTF_Quit();
}

void GUI::processSystemEvents(const SDL_Event& event) {
    // Aqui ficam apenas os eventos globais da aplicação, como fechar a janela e salvar por atalho.
    switch (event.type) {
        case SDL_EVENT_QUIT:
            running_ = false;
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (main_window_ && event.window.windowID == SDL_GetWindowID(main_window_)) {
                running_ = false;
            }
            break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_S) {
                saveCurrentImage();
            }
            break;

        default:
            break;
    }
}

void GUI::processButtonClicks() {
    if (equalize_button_.wasClicked()) {
        handleEqualizeButtonClick();
    }

    if (save_button_.wasClicked()) {
        handleSaveButtonClick();
    }

    if (exit_button_.wasClicked()) {
        handleExitButtonClick();
    }
}

void GUI::handleEqualizeButtonClick() {
    // O botão alterna entre a imagem equalizada e a base em tons de cinza.
    if (image_processor_.getIsEqualized()) {
        image_processor_.restoreOriginal();
        equalize_button_.setText("Equalizar");
        is_currently_equalized_ = false;
    } else {
        image_processor_.equalizeHistogram();
        equalize_button_.setText("Original");
        is_currently_equalized_ = true;
    }

    histogram_.calculate(image_processor_.getCurrentImage());
    updateImageTexture();
}

void GUI::handleSaveButtonClick() {
    saveCurrentImage();
}

void GUI::handleExitButtonClick() {
    running_ = false;
}

void GUI::saveCurrentImage() {
    constexpr std::string_view output_filename = "output_image.png";

    // O salvamento sempre sobrescreve o mesmo arquivo para simplificar a validação.
    if (image_processor_.saveImage(output_filename.data())) {
        last_save_path_ = std::string(output_filename);
        last_save_status_ = "OK";
        std::cout << "Imagem salva com sucesso em: " << output_filename << '\n';
    } else {
        last_save_path_ = std::string(output_filename);
        last_save_status_ = "ERRO";
        std::cerr << "Falha ao salvar imagem em: " << output_filename << '\n';
    }
}

void GUI::renderImageArea() {
    // A área da esquerda existe apenas para a imagem e mantém a moldura mesmo sem textura válida.
    const float left_region_width = imageAreaWidth(main_window_width_);
    const SDL_FRect image_panel = {
        static_cast<float>(ui_constants::MAIN_IMAGE_PADDING / 2),
        static_cast<float>(ui_constants::MAIN_IMAGE_PADDING / 2),
        left_region_width - static_cast<float>(ui_constants::MAIN_IMAGE_PADDING),
        static_cast<float>(main_window_height_ - ui_constants::MAIN_IMAGE_PADDING)
    };
    drawPanel(main_renderer_, image_panel);

    if (!image_texture_) {
        return;
    }

    const SDL_FRect image_rect = computeFittedImageRect(
        image_panel,
        image_processor_.getWidth(),
        image_processor_.getHeight()
    );

    SDL_SetRenderDrawColor(main_renderer_,
                           ui_constants::BACKGROUND_MAIN.r,
                           ui_constants::BACKGROUND_MAIN.g,
                           ui_constants::BACKGROUND_MAIN.b,
                           255);
    SDL_RenderFillRect(main_renderer_, &image_rect);
    SDL_RenderTexture(main_renderer_, image_texture_, nullptr, &image_rect);

    SDL_SetRenderDrawColor(main_renderer_,
                           ui_constants::IMAGE_FRAME.r,
                           ui_constants::IMAGE_FRAME.g,
                           ui_constants::IMAGE_FRAME.b,
                           ui_constants::IMAGE_FRAME.a);
    SDL_RenderRect(main_renderer_, &image_rect);
}

void GUI::renderSidebar() {
    // O painel lateral organiza o histograma, as estatísticas, os dados técnicos e os botões.
    const float panel_origin_x = sidebarOriginX(main_window_width_);
    const SDL_FRect sidebar_background = {
        panel_origin_x,
        0.0f,
        static_cast<float>(SIDEBAR_WIDTH),
        static_cast<float>(main_window_height_)
    };

    SDL_SetRenderDrawColor(main_renderer_,
                           ui_constants::BACKGROUND_SECONDARY.r,
                           ui_constants::BACKGROUND_SECONDARY.g,
                           ui_constants::BACKGROUND_SECONDARY.b,
                           ui_constants::BACKGROUND_SECONDARY.a);
    SDL_RenderFillRect(main_renderer_, &sidebar_background);

    drawText(main_renderer_, "Histograma", static_cast<int>(panel_origin_x + 18.0f), 16, ui_constants::TITLE_COLOR);
    drawSeparator(main_renderer_, panel_origin_x + 18.0f, 38.0f, ui_constants::PANEL_WIDTH);

    const SDL_FRect histogram_panel = {
        panel_origin_x + ui_constants::PANEL_PADDING,
        ui_constants::HISTOGRAM_TOP,
        ui_constants::PANEL_WIDTH,
        ui_constants::HISTOGRAM_HEIGHT
    };
    drawPanel(main_renderer_, histogram_panel);

    histogram_.drawWithOverlay(
        main_renderer_,
        static_cast<int>(panel_origin_x + ui_constants::PANEL_PADDING + ui_constants::PANEL_INNER_PADDING),
        static_cast<int>(ui_constants::HISTOGRAM_TOP + ui_constants::PANEL_INNER_PADDING),
        static_cast<int>(ui_constants::PANEL_WIDTH - ui_constants::PANEL_INNER_PADDING * 2.0f),
        static_cast<int>(ui_constants::HISTOGRAM_HEIGHT - ui_constants::PANEL_INNER_PADDING * 2.0f),
        original_histogram_,
        ui_constants::HISTOGRAM_OVERLAY
    );

    renderHistogramInformation();
    renderTechnicalInformation();

    equalize_button_.draw(main_renderer_, font_);
    save_button_.draw(main_renderer_, font_);
    exit_button_.draw(main_renderer_, font_);
}

void GUI::renderHistogramInformation() {
    if (!font_) return;

    // As estatísticas mostram o estado atual e a referência original em cinza lado a lado.
    const int origin_x = static_cast<int>(sidebarOriginX(main_window_width_));
    drawText(main_renderer_, "Estatísticas", origin_x + 18, static_cast<int>(ui_constants::STATS_TITLE_TOP), ui_constants::TITLE_COLOR);
    drawSeparator(main_renderer_, static_cast<float>(origin_x + 18), ui_constants::STATS_TITLE_TOP + 22.0f, ui_constants::PANEL_WIDTH);

    const SDL_FRect stats_panel = {
        static_cast<float>(origin_x) + ui_constants::PANEL_PADDING,
        ui_constants::STATS_TOP,
        ui_constants::PANEL_WIDTH,
        ui_constants::STATS_HEIGHT
    };
    drawPanel(main_renderer_, stats_panel);

    const int label_x = origin_x + 32;
    const int value_x = origin_x + 172;
    const int first_line_y = static_cast<int>(ui_constants::STATS_TOP + 20.0f);

    const std::string current_mean = std::to_string(static_cast<int>(histogram_.getMean())) +
                                     " (" + histogram_.getIntensityClassification() + ")";
    drawText(main_renderer_, "Média atual", label_x, first_line_y, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, current_mean, value_x, first_line_y, ui_constants::TEXT_PRIMARY);

    const std::string current_std_dev = std::to_string(static_cast<int>(histogram_.getStdDev())) +
                                        " (" + histogram_.getContrastClassification() + ")";
    drawText(main_renderer_, "Desvio atual", label_x, first_line_y + 22, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, current_std_dev, value_x, first_line_y + 22, ui_constants::TEXT_PRIMARY);

    const std::string original_mean = std::to_string(static_cast<int>(original_histogram_.getMean())) +
                                      " (" + original_histogram_.getIntensityClassification() + ")";
    drawText(main_renderer_, "Média original", label_x, first_line_y + 44, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, original_mean, value_x, first_line_y + 44, ui_constants::TEXT_SECONDARY);

    const std::string original_std_dev = std::to_string(static_cast<int>(original_histogram_.getStdDev())) +
                                         " (" + original_histogram_.getContrastClassification() + ")";
    drawText(main_renderer_, "Desvio original", label_x, first_line_y + 66, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, original_std_dev, value_x, first_line_y + 66, ui_constants::TEXT_SECONDARY);

    drawText(main_renderer_, "Atalho salvar", label_x, first_line_y + 88, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, "S", value_x, first_line_y + 88, ui_constants::TEXT_MUTED);
}

void GUI::renderTechnicalInformation() {
    if (!font_) return;

    // Esse bloco resume o arquivo carregado, o estado do processamento e o último salvamento.
    const int origin_x = static_cast<int>(sidebarOriginX(main_window_width_));
    drawText(main_renderer_, "Informações técnicas", origin_x + 18, static_cast<int>(ui_constants::TECH_TITLE_TOP), ui_constants::TITLE_COLOR);
    drawSeparator(main_renderer_, static_cast<float>(origin_x + 18), ui_constants::TECH_TITLE_TOP + 22.0f, ui_constants::PANEL_WIDTH);

    const SDL_FRect technical_panel = {
        static_cast<float>(origin_x) + ui_constants::PANEL_PADDING,
        ui_constants::TECH_TOP,
        ui_constants::PANEL_WIDTH,
        ui_constants::TECH_HEIGHT
    };
    drawPanel(main_renderer_, technical_panel);

    const int label_x = origin_x + 32;
    const int value_x = origin_x + 172;
    const int first_line_y = static_cast<int>(ui_constants::TECH_TOP + 20.0f);

    drawText(main_renderer_, "Arquivo", label_x, first_line_y, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, fileNameFromPath(current_image_path_), value_x, first_line_y, ui_constants::TEXT_PRIMARY);

    const std::string dimensions = std::to_string(image_processor_.getWidth()) + "x" +
                                   std::to_string(image_processor_.getHeight());
    drawText(main_renderer_, "Dimensão", label_x, first_line_y + 20, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, dimensions, value_x, first_line_y + 20, ui_constants::TEXT_PRIMARY);

    drawText(main_renderer_, "Formato", label_x, first_line_y + 40, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, detectImageFormatLabel(current_image_path_), value_x, first_line_y + 40, ui_constants::TEXT_PRIMARY);

    drawText(main_renderer_, "Original em cinza", label_x, first_line_y + 60, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, image_was_originally_gray_ ? "Sim" : "Não", value_x, first_line_y + 60, ui_constants::TEXT_PRIMARY);

    drawText(main_renderer_, "Estado", label_x, first_line_y + 80, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, is_currently_equalized_ ? "Equalizada" : "Original", value_x, first_line_y + 80, ui_constants::TEXT_PRIMARY);

    const std::string last_save = last_save_path_.empty()
        ? "(nenhum)"
        : fileNameFromPath(last_save_path_) + " [" + last_save_status_ + "]";
    drawText(main_renderer_, "Último salvamento", label_x, first_line_y + 100, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, last_save, value_x, first_line_y + 100, ui_constants::TEXT_PRIMARY);

    drawText(main_renderer_, "Driver SDL", label_x, first_line_y + 120, ui_constants::TEXT_SECONDARY);
    drawText(main_renderer_, currentVideoDriverLabel(), value_x, first_line_y + 120, ui_constants::TEXT_MUTED);
}
