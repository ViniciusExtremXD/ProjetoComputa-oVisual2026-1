# Projeto Computação Visual 2026

Universidade Presbiteriana Mackenzie  
Faculdade de Computação e Informática  
Disciplina: Computação Visual

Integrantes:
- Rodrigo Rosalles - 10409316
- Vinícius Magno - 10401365
- Natalia Teixeira - 10395853

## Contexto da atividade

Este repositório reúne a implementação do Projeto 1 da disciplina de Computação Visual. O trabalho parte do enunciado definido em Projeto 1 (Proj1).pdf e exige o carregamento de uma imagem pela linha de comando, a conversão para tons de cinza, a análise do histograma e a disponibilização de uma interface para inspeção e salvamento do resultado.

## Objetivo do trabalho

O objetivo do projeto é processar imagens coloridas em C++17, produzir a versão em tons de cinza com a fórmula indicada no enunciado, calcular estatísticas associadas ao histograma e oferecer uma forma direta de comparar o estado original em cinza com a versão equalizada.

## Visão geral da solução

A aplicação foi organizada em três partes principais. ImageProcessor concentra a carga da imagem, a detecção de cinza, a conversão, a equalização e o salvamento. Histogram calcula a distribuição das intensidades, a média, o desvio padrão e também exporta os dados para arquivos auxiliares. GUI integra esses resultados em uma janela única de 1280x720, com a imagem à esquerda e o painel técnico à direita.

## Funcionalidades implementadas

- carregamento de imagens PNG, JPG e BMP;
- detecção de imagens que já estão em tons de cinza;
- conversão RGB para tons de cinza com a fórmula do enunciado;
- cálculo do histograma da imagem exibida e da imagem base em cinza;
- cálculo de média de intensidade e desvio padrão;
- classificação textual de intensidade e contraste;
- equalização do histograma com reversão para a imagem base;
- salvamento da imagem atual pelo botão Salvar e pela tecla S;
- execução com interface gráfica e em modo headless (--nogui).

## Estrutura do projeto

- src/main.cpp: ponto de entrada, leitura dos argumentos e escolha entre GUI e modo headless.
- src/ImageProcessor.cpp: carga da imagem, conversão para cinza, equalização, restauração e salvamento.
- src/Histogram.cpp: cálculo do histograma, estatísticas e exportação dos dados.
- src/GUI.cpp: janela SDL, renderização da imagem, painel lateral e tratamento de eventos.
- include/: declarações das classes principais.
- assets/: imagens usadas nos testes.

## Bibliotecas utilizadas

- SDL3 para criação da janela, renderer e tratamento de eventos;
- SDL3_image para leitura e gravação de imagens;
- SDL3_ttf para renderização dos textos da interface.

## Como compilar

No WSL Ubuntu, com as bibliotecas SDL instaladas, a compilação pode ser feita com:

```bash
make clean
make check-deps
make
```

Se pkg-config não localizar as bibliotecas em /usr/local, é possível exportar:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

## Como executar

Modo gráfico:

```bash
./build/main assets/gato.png
```

Modo headless:

```bash
./build/main --nogui assets/gato.png
```

Também é possível usar os alvos do Makefile:

```bash
make run IMAGE=assets/gato.png
make test-headless IMAGE=assets/gato.png
```

## Descrição da interface

A interface foi mantida em uma única janela. A área esquerda é dedicada apenas à imagem processada, com ajuste de escala e centralização. A lateral direita concentra o histograma, as estatísticas principais, as informações técnicas da imagem e três botões (Equalizar ou Original, Salvar e Sair).

## Explicação da conversão para tons de cinza

A conversão segue a combinação ponderada pedida no enunciado:

Y = 0.2125R + 0.7154G + 0.0721B

O cálculo é aplicado a cada pixel após a imagem ser convertida para um formato interno compatível com o pipeline do SDL. Se a imagem de entrada já estiver em cinza, o programa preserva essa condição e evita uma conversão desnecessária.

## Explicação do histograma

O histograma contabiliza a frequência de cada nível de intensidade entre 0 e 255. A partir dessa distribuição, o programa calcula a média de intensidade e o desvio padrão, que são exibidos na interface e também exportados no modo headless. A interface sobrepõe o histograma da imagem atual ao histograma da imagem base em cinza para facilitar a comparação visual.

## Explicação da equalização

A equalização é feita sobre a imagem base em tons de cinza. O algoritmo calcula o histograma, monta a distribuição acumulada e gera um mapeamento de intensidades para redistribuir os níveis de cinza. O resultado amplia o uso da faixa dinâmica da imagem. O botão Original restaura a imagem base em cinza sem recarregar o arquivo.

## Saída gerada pelo programa

No modo gráfico, o salvamento gera output_image.png com o estado atual da imagem. No modo headless, além desse arquivo, o programa cria uma pasta output_<nome-da-imagem>/ com:

- a imagem em tons de cinza;
- o CSV do histograma;
- o resumo textual com média e desvio padrão;
- a imagem do gráfico do histograma;
- a mesma exportação para a imagem base original em cinza.

## Contribuição individual dos integrantes

- Rodrigo Rosalles: organização da interface gráfica, composição do layout e integração entre eventos e atualização visual.
- Vinícius Magno: implementação do pipeline de processamento, carga e salvamento das imagens, cálculo do histograma e equalização.
- Natalia Teixeira: revisão textual, verificação final dos fluxos de uso e consolidação da documentação para entrega.

## Considerações finais

O projeto atende ao fluxo principal pedido no enunciado e mantém separação clara entre processamento, análise estatística e interface. A versão atual prioriza legibilidade do código, comentários objetivos e documentação técnica compatível com uma entrega acadêmica.
