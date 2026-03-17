# Projeto 1 - Processamento de Imagens

Universidade Presbiteriana Mackenzie  
Faculdade de Computação e Informática  
Disciplina: Computação Visual

Projeto desenvolvido em C++17 com SDL3, SDL3_image e SDL3_ttf. O programa recebe o caminho de uma imagem pela linha de comando, converte a imagem colorida para escala de cinza com a fórmula exigida no enunciado, exibe o histograma em uma segunda janela, permite equalizar o histograma e salvar a imagem atual em `output_image.png`.

## Integrantes

- Rodrigo Rosalles - 10409316
- Vinícius Magno - 10401365

## Contribuição individual

- Rodrigo Rosalles: estruturação da interface gráfica com duas janelas, renderização dos botões, integração entre GUI e processamento e organização geral do projeto.
- Vinícius Magno: carregamento e salvamento de imagens, conversão para escala de cinza, cálculo e exportação do histograma, equalização e ajustes de build e documentação.

## Requisitos implementados

- Carregamento de PNG, JPG e BMP com tratamento de erro.
- Detecção de imagens já em escala de cinza.
- Conversão para escala de cinza usando a fórmula do enunciado: `Y = 0.2125R + 0.7154G + 0.0721B`.
- Uso da imagem em escala de cinza como base para histograma, equalização e reversão.
- GUI com janela principal adaptada ao tamanho da imagem e janela secundária fixa ao lado.
- Exibição do histograma com média de intensidade e desvio padrão classificados.
- Botão desenhado com primitivas SDL, com estados visuais normal, hover e pressionado.
- Equalização com reversão para a versão original em escala de cinza sem recarregar o arquivo.
- Salvamento da imagem atual com a tecla `S` em `output_image.png`.

## Requisitos técnicos

- Linguagem: C++17
- Compilador alvo: g++
- Bibliotecas: SDL3, SDL3_image e SDL3_ttf

## Compilação no WSL Ubuntu

Após instalar as dependências do SDL em `/usr/local`, exporte:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

Compile com:

```bash
make clean
make check-deps
make
```

## Execução

O programa sempre exige uma imagem válida por linha de comando.

Use imagens em `assets/` como amostra/demo. O programa funciona com qualquer caminho de imagem informado por argumento. A pasta `tmp_test_assets/` é apenas para automação local de testes e não faz parte da entrega final.

### Assets de amostra (opcional)

Os arquivos abaixo existem no repositório e podem ser usados nos testes:

- `assets/gato.png`
- `assets/paisagem.png`
- `assets/carro.png`

### GUI

```bash
./build/main caminho/para/imagem.png
```

Ou pelo Makefile:

```bash
make run IMAGE=caminho/para/imagem.png
```

Exemplo recomendado:

```bash
make run IMAGE=assets/gato.png
./build/main assets/paisagem.png
```

### Headless

```bash
./build/main --nogui caminho/para/imagem.png
```

Ou pelo Makefile:

```bash
make test-headless IMAGE=caminho/para/imagem.png
```

Exemplo recomendado:

```bash
make test-headless IMAGE=assets/carro.png
./build/main --nogui assets/carro.png
```

## Saídas geradas no modo headless

- `output_image.png`
- `output_<nome-da-imagem>/`
- `<nome>_grayscale.png`
- `<nome>_histograma.csv`
- `<nome>_stats.txt`
- `<nome>_histograma.png`
- `<nome>_original_histograma.csv`
- `<nome>_original_stats.txt`
- `<nome>_original_histograma.png`

## Controles da interface

1. `Abrir`: carrega uma nova imagem.
2. `Equalizar`: equaliza a imagem atual.
3. `Original`: restaura a imagem base em escala de cinza.
4. `Salvar`: grava a imagem atual no caminho escolhido.
5. Tecla `S`: sobrescreve `output_image.png` com a imagem atualmente exibida.

## Observações

- A GUI depende de uma fonte válida do sistema. No Linux, o programa tenta fontes comuns como DejaVu Sans, Liberation Sans, Noto Sans e FreeSans. Se nenhuma estiver disponível, a GUI falha explicitamente.
- A pasta `examples/` pode existir como opcional para imagens demo, mas o fluxo oficial continua sendo informar a imagem explicitamente pela linha de comando.
- O histórico Git do repositório deve permanecer coerente com o grupo oficial para a entrega formal.
