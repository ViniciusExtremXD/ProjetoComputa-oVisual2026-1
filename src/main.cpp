// Ponto de entrada da aplicação.
// Aqui ficam a leitura dos argumentos, a inicialização do SDL
// e a escolha entre a interface gráfica e o modo headless.
// Integrantes:
// Rodrigo Rosalles - 10409316
// Vinícius Magno - 10401365
// Natalia Teixeira - 10395853

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_video.h>
#include <SDL3_image/SDL_image.h>

#include "../include/GUI.h"
#include "../include/Histogram.h"

namespace {

struct SdlInitializationResult {
    bool is_initialized{false};
    bool used_fallback{false};
    std::string driver_name;
};

namespace constants {
    constexpr std::string_view NO_GUI_FLAG = "--nogui";
    constexpr std::string_view OUTPUT_IMAGE_NAME = "output_image.png";
    constexpr std::string_view DEFAULT_IMAGE_NAME = "imagem";
    constexpr std::string_view OUTPUT_DIR_PREFIX = "output_";
    constexpr std::string_view GRAYSCALE_SUFFIX = "_grayscale.png";
    constexpr std::string_view HISTOGRAM_SUFFIX = "_histograma.csv";
    constexpr std::string_view STATS_SUFFIX = "_stats.txt";
    constexpr std::string_view PLOT_SUFFIX = "_histograma.png";
    constexpr std::string_view ORIGINAL_PREFIX = "_original";
}

void logAvailableVideoDrivers() noexcept {
    const int driver_count = SDL_GetNumVideoDrivers();

    if (driver_count < 0) {
        std::cerr << "[SDL] Não foi possível listar drivers de vídeo: "
                  << SDL_GetError() << '\n';
        return;
    }

    std::cout << "[SDL] Drivers de vídeo disponíveis (" << driver_count << "):";

    if (driver_count == 0) {
        std::cout << " nenhum\n";
        return;
    }

    std::cout << '\n';
    for (int i = 0; i < driver_count; ++i) {
        const char* driver_name = SDL_GetVideoDriver(i);
        std::cout << "  - " << (driver_name ? driver_name : "(desconhecido)") << '\n';
    }
}

[[nodiscard]] SdlInitializationResult initializeSdlWithFallback(Uint32 initialization_flags) noexcept {
    SdlInitializationResult result;
    logAvailableVideoDrivers();

    // Primeiro tenta subir o vídeo com um driver real, que é o caminho esperado da GUI.
#if SDL_VERSION_ATLEAST(3, 0, 0)
    const bool init_success = SDL_Init(initialization_flags);
#else
    const bool init_success = SDL_Init(initialization_flags) == 0;
#endif

    if (init_success) {
        result.is_initialized = true;
        const char* current_driver = SDL_GetCurrentVideoDriver();
        result.driver_name = current_driver ? current_driver : "(desconhecido)";
        std::cout << "[SDL] Inicialização bem-sucedida com driver '"
                  << result.driver_name << "'\n";
        return result;
    }

    // Se isso falhar, o erro é guardado para diagnóstico antes do fallback.
    const std::string first_error = SDL_GetError();
    std::cerr << "[SDL] Falha ao inicializar subsistema de vídeo: "
              << (first_error.empty() ? "(erro não informado)" : first_error) << '\n';

    SDL_Quit();

    // O driver dummy mantém o pipeline funcional para execuções sem janela.
    std::cout << "[SDL] Tentando fallback com driver 'dummy'...\n";

#if defined(SDL_HINT_VIDEO_DRIVER)
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
#elif defined(SDL_HINT_VIDEODRIVER)
    SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
#endif

#if SDL_VERSION_ATLEAST(3, 0, 0)
    const bool fallback_success = SDL_Init(initialization_flags);
#else
    const bool fallback_success = SDL_Init(initialization_flags) == 0;
#endif

    if (fallback_success) {
        result.is_initialized = true;
        result.used_fallback = true;
        const char* fallback_driver = SDL_GetCurrentVideoDriver();
        result.driver_name = fallback_driver ? fallback_driver : "dummy";
        std::cout << "[SDL] Inicialização usando driver de fallback '"
                  << result.driver_name << "'.\n";
    } else {
        const std::string fallback_error = SDL_GetError();
        std::cerr << "[SDL] Falha também no fallback: "
                  << (fallback_error.empty() ? "(erro não informado)" : fallback_error) << '\n';
    }

    return result;
}

void initializeSdlImage() noexcept {
#if defined(SDL_IMAGE_VERSION_ATLEAST)
#if !SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    constexpr int image_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(image_flags) & image_flags) != image_flags) {
        std::cerr << "Aviso: falha ao inicializar SDL_image: "
                  << IMG_GetError() << '\n';
    }
#endif
#elif defined(IMG_Init)
    constexpr int image_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(image_flags) & image_flags) != image_flags) {
        std::cerr << "Aviso: falha ao inicializar SDL_image: "
                  << IMG_GetError() << '\n';
    }
#endif
}

void cleanupSdlImage() noexcept {
#if defined(SDL_IMAGE_VERSION_ATLEAST)
#if !SDL_IMAGE_VERSION_ATLEAST(3, 0, 0)
    IMG_Quit();
#endif
#elif defined(IMG_Quit)
    IMG_Quit();
#endif
}

void saveHistogramData(const Histogram& histogram,
                       const std::filesystem::path& output_directory,
                       const std::string& base_filename,
                       const std::string& file_suffix = "") {
    const std::string filename_prefix = base_filename + file_suffix;

    // O modo headless exporta a mesma análise em três formas: CSV, resumo e imagem do gráfico.
    const auto csv_path = output_directory / (filename_prefix + std::string(constants::HISTOGRAM_SUFFIX));
    if (histogram.saveCSV(csv_path.string())) {
        std::cout << "Histograma salvo em: " << csv_path << '\n';
    } else {
        std::cerr << "Aviso: falha ao salvar CSV do histograma em " << csv_path << '\n';
    }

    const auto summary_path = output_directory / (filename_prefix + std::string(constants::STATS_SUFFIX));
    if (histogram.saveSummary(summary_path.string())) {
        std::cout << "Resumo estatístico salvo em: " << summary_path << '\n';
    } else {
        std::cerr << "Aviso: falha ao salvar resumo do histograma em " << summary_path << '\n';
    }

    const auto plot_path = output_directory / (filename_prefix + std::string(constants::PLOT_SUFFIX));
    if (histogram.savePlotImage(plot_path.string())) {
        std::cout << "Imagem do histograma salva em: " << plot_path << '\n';
    } else {
        std::cerr << "Aviso: falha ao renderizar o histograma em " << plot_path << '\n';
    }
}

[[nodiscard]] int processImageHeadless(const std::string& image_path) {
    try {
        // O fluxo headless carrega a imagem, gera a base em cinza e salva o resultado atual.
        ImageProcessor image_processor;
        if (!image_processor.loadImage(image_path.c_str())) {
            std::cerr << "Erro ao carregar imagem em modo --nogui\n";
            return 1;
        }

        // Se a imagem processada não existir, não faz sentido continuar exportando os artefatos.
        if (!image_processor.getCurrentImage()) {
            std::cerr << "Imagem atual inválida após processamento\n";
            return 1;
        }

        std::cout << "Imagem original em escala de cinza: "
                  << (image_processor.isOriginalGrayscale() ? "sim" : "não") << '\n';

        if (!image_processor.saveImage(std::string(constants::OUTPUT_IMAGE_NAME).c_str())) {
            std::cerr << "Falha ao salvar " << constants::OUTPUT_IMAGE_NAME << '\n';
            return 1;
        }

        // Os dois histogramas permitem registrar a imagem atual e a base em cinza separadamente.
        Histogram current_histogram;
        current_histogram.calculate(image_processor.getCurrentImage());

        Histogram original_histogram;
        original_histogram.calculate(image_processor.getGrayscaleImage());

        if (current_histogram.getTotalPixels() > 0) {
            std::cout << "Histograma (processada): média=" << current_histogram.getMean()
                      << " (" << current_histogram.getIntensityClassification()
                      << ") desvio=" << current_histogram.getStdDev()
                      << " (" << current_histogram.getContrastClassification() << ")\n";

            // O nome da pasta de saída acompanha o nome do arquivo informado na linha de comando.
            const std::filesystem::path input_path(image_path);
            std::string base_name = input_path.stem().string();
            if (base_name.empty()) {
                base_name = std::string(constants::DEFAULT_IMAGE_NAME);
            }

            const auto output_directory = std::filesystem::path(
                std::string(constants::OUTPUT_DIR_PREFIX) + base_name
            );

            std::error_code directory_error;
            std::filesystem::create_directories(output_directory, directory_error);

            if (directory_error) {
                std::cerr << "Aviso: não foi possível criar diretório de saída '"
                          << output_directory << "': " << directory_error.message() << '\n';
                return 1;
            }

            saveHistogramData(current_histogram, output_directory, base_name);

            const std::filesystem::path processed_image_path(constants::OUTPUT_IMAGE_NAME);
            if (std::filesystem::exists(processed_image_path)) {
                const auto copy_path = output_directory / (base_name + std::string(constants::GRAYSCALE_SUFFIX));
                std::error_code copy_error;
                std::filesystem::copy_file(
                    processed_image_path,
                    copy_path,
                    std::filesystem::copy_options::overwrite_existing,
                    copy_error
                );

                if (!copy_error) {
                    std::cout << "Imagem em escala de cinza copiada para: " << copy_path << '\n';
                } else {
                    std::cerr << "Aviso: não foi possível copiar imagem processada: "
                              << copy_error.message() << '\n';
                }
            }

            if (original_histogram.getTotalPixels() > 0) {
                saveHistogramData(
                    original_histogram,
                    output_directory,
                    base_name,
                    std::string(constants::ORIGINAL_PREFIX)
                );
            } else {
                std::cerr << "Aviso: não foi possível calcular histograma original da imagem.\n";
            }
        } else {
            std::cerr << "Aviso: não foi possível calcular histograma da imagem processada.\n";
        }

        std::cout << "Modo --nogui: imagem processada e salva em "
                  << constants::OUTPUT_IMAGE_NAME << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "Erro em modo --nogui: " << exception.what() << '\n';
        return 1;
    }
}

void printUsage(const char* program_name) noexcept {
    std::cerr << "Uso: " << program_name << " caminho_da_imagem.ext\n";
    std::cerr << "  ou: " << program_name << " --nogui caminho_da_imagem.ext\n";
}

[[nodiscard]] std::optional<std::pair<bool, std::string>> parseCommandLineArguments(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return std::nullopt;
    }

    const std::string_view first_arg(argv[1]);

    if (first_arg == constants::NO_GUI_FLAG) {
        if (argc != 3) {
            std::cerr << "Uso: " << argv[0] << " --nogui caminho_da_imagem.ext\n";
            return std::nullopt;
        }
        return std::make_pair(true, std::string(argv[2]));
    }

    return std::make_pair(false, std::string(argv[1]));
}

} // namespace

int main(int argc, char* argv[]) {
    // A aplicação sempre depende de um caminho de imagem válido.
    const auto command_args = parseCommandLineArguments(argc, argv);
    if (!command_args) {
        return 1;
    }

    const auto [no_gui_mode, image_path] = *command_args;

    // O SDL é necessário tanto para a GUI quanto para o tratamento das superfícies no restante do programa.
    constexpr Uint32 sdl_flags = SDL_INIT_VIDEO | SDL_INIT_EVENTS;
    const auto sdl_initialization = initializeSdlWithFallback(sdl_flags);

    if (!sdl_initialization.is_initialized) {
        std::cerr << "Erro crítico: SDL não pôde ser inicializado. "
                  << "Verifique drivers de vídeo ou execute com --nogui.\n";
        return 1;
    }

    if (sdl_initialization.used_fallback) {
        std::cerr << "Aviso: driver '" << sdl_initialization.driver_name
                  << "' não suporta janelas. Modo GUI indisponível nesta execução.\n";
    }

    initializeSdlImage();

    // O wrapper evita retorno prematuro deixando subsistemas abertos.
    const auto sdl_cleanup = std::unique_ptr<int, void(*)(int*)>(
        new int(1),
        [](int* ptr) {
            delete ptr;
            cleanupSdlImage();
            SDL_Quit();
        }
    );

    // No modo headless, a execução termina depois de gerar os arquivos de saída.
    if (no_gui_mode) {
        return processImageHeadless(image_path);
    }

    // A interface só pode abrir quando o SDL conseguiu um driver de vídeo real.
    if (sdl_initialization.used_fallback) {
        std::cerr << "Erro: modo GUI requer um driver de vídeo real. "
                  << "Execute aplicações headless com --nogui.\n";
        return 1;
    }

    // Fora do headless, o restante do trabalho fica concentrado no loop da GUI.
    try {
        GUI gui(image_path);
        gui.run();
    } catch (const std::exception& exception) {
        std::cerr << "Erro: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
