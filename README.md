# INTERRUPTOR INTELIGENTE COM ESP32 E HC-SR04

## Índice
- [Motivação](#motivação)
- [Contexto de Implantação](#contexto-de-implantação)
- [Componentes](#componentes)
- [Plataformas](#plataformas)
- [Diagrama](#diagrama)
- [Como utilizar o programa baixando os arquivos (VSCode)](#como-utilizar-o-programa-baixando-os-arquivos-vscode)
- [Funcionamento](#funcionamento)
- [Valores importantes a serem alterados](#valores-importantes-a-serem-alterados)
- [Cuidados a serem tomados](#cuidados-a-serem-tomados)
- [Considerações finais](#considerações-finais)


## Motivação

<p align="justify">Este trabalho se deu no contexto de um projeto de extensão (dispenso detalhes sobre a instituição) da disciplina de Microcontroladores, do curso de Ciência da Computação, em 2025, onde tivemos que procurar suprir as necessidades de uma comunidade específica à escolha do aluno utilizando os conhecimentos adquiridos.</p>

## Contexto de implantação

<p align="justify">O dispositivo programado foi pensado para um corredor estreito (suporta apenas uma pessoa por vez), com apenas dois pontos de acesso, que possui apenas uma lâmpada, sendo que o dispositivo só será utilizado a noite, em apenas um dos acessos, realizando comunicação com uma lâmpada inteligente via SinricPro e Google Home.</p>

## Componentes


- 1 Módulo ESP32;
- 1 Sensor de distância HC-SR04;
- 1 Fonte de alimentação de 5v;
- Fios necessários para fazer as ligações.

<p align="justify">
    
*Atenção!! Cuidado ao fazer as conexões. Garanta que a placa e o sensor trabalhem com a mesma tensão. Caso tenha adquirido um módulo como o DOIT ESP32 DEVKIT V1, que opera a 3v, procure fazer algo como um divisor de tensão para evitar eventuais problemas, ou troque o sensor por outro que trabalhe a 3v.*

</p>

## Plataformas

Utilizei o PlatformIO para a facilitar o processo de codificar tudo o que foi projetado, então você pode encontrar arquivos de configurações relativos a ele no repositório. Para mais informações, consulte:

https://platformio.org/

E para comunicação com outros dispositivos, utilizei o SinricPro. Para saber mais, acesse:

https://sinric.pro/

### Diagrama

![simulacao-no-wokwi](https://github.com/user-attachments/assets/7ce927a8-6f04-4239-8682-259da6981f10)
*Criado em https://wokwi.com/*.

## Como utilizar o programa baixando os arquivos (VSCode)

1. Compre os devidos componentes e faça as ligações (de preferência, em uma protoboard);
2. [Baixe o VSCode](https://code.visualstudio.com/download) e instale em sua máquina;
3. Instale a extensão do PlatformIO no VSCode (veja [como instalar extensões no VSCode](https://code.visualstudio.com/docs/getstarted/extensions));
4. Baixe os arquivos deste repositório (consulte [como baixar arquivos do repositório](https://docs.github.com/pt/get-started/start-your-journey/downloading-files-from-github));
5. Abra O PlatformIO Home:

![tela-inicial-do-vscode](https://github.com/user-attachments/assets/be7f2191-1cea-48d4-9703-87d93add476f)
*Tela inicial do VSCode - Clicar no ícone do PlatformIO*.

6. Escolha "Selecionar uma pasta":

![platformio-home](https://github.com/user-attachments/assets/f2bfd0cb-e5e1-4cc8-9218-e92d20ff30f6)
*PlatformIO Home - Escolher "Selecionar uma pasta"*.

7. Selecione o diretório do projeto;
8. Conecte seu dispositivo em uma porta do seu computador (certifique-se de estar usando um USB adequado conectado a uma porta que suporte a comunicação serial);
9. Vá novamente no ícone do PlatformIO e abra o PlatformIO Home;
10. Vá para *src/main.cpp* no *EXPLORER* e faça as alterações necessárias no código. *Veja o que deve ser alterado* e *os cuidados a serem tomados* nas seções seguintes.
11. Vá em *Upload and Monitor* dentro de *PROJECT TASKS*:
    
![platformio-home](https://github.com/user-attachments/assets/f7717962-940e-4322-9bad-0b191e10ccec)
*PlatiformIO Home - Clicar em Upload and Monitor*.

*O acompanhamento do funcionamento será feito pelo terminal do VSCode.*

## Funcionamento

<p align="justify">
A ideia básica por trás de tudo é bastante simples: o sensor detecta uma variação na distância de uma pessoa que entra ou sai do corredor enquanto ela estiver em uma faixa definida de detecção da qual quando a pessoa sair, o sensor verifica a variação total, definindo se a pessoa saiu ou entrou. Cada entrada ou saída é contabilizada numa variável de contagem. Enquanto ela estiver acima de 0, a lâmpada permanecerá acesa, caso contrário, ficará apagada. Fora isso, temos apenas o gerenciamento de conexões e de falhas.
</p>

## Valores importantes a serem alterados

<p align="justify">
    
*No início do código você pode notar algumas diretivas de pré-compilação e constantes que ditam como o seu dispositivo vai se comportar, devendo ser ajustados de acordo com sua necessidade. Elas que serão listadas a seguir.*

- **`BAUD_RATE`**: define a velocidade com que os bits são enviados durante a comunicação serial. Isso pode variar de placa para placa.

- **`TAMANHO_STACK_TASK_PRINCIPAL`**: especifica quantas palavras serão utilizadas na task principal do programa. Varia de processador para processador.

- **`TAMANHO_STACK_VERIFICADORES_CONEXAO`**: especifica quntas palavras esrão utilizadas nas tasks de verificação da conexão com o SinricPro e com o WiFi. Varia de processador para processador.

- **`TRIG`**: pino conectado ao trigger do sensor HC-SR04.

- **`ECCHO`**: pino conectado ao eccho do sensor HC-SR04.

- **`FAIXA_DETECCAO_CM`**: define o tamanho (em cm) da faixa de detecção, dentro da qual as variações serão consideradas. Qualquer coisa fora da faixa de detecção será ignorada. Note que ela começará a partir de **`INICIO_FAIXA_DETECCAO_CM`**.

- **`INICIO_FAIXA_DETECCAO_CM`**: início da faixa de detecção (em cm). Qualquer coisa quenão esteja entre **`INICIO_FAIXA_DETECCAO_CM`** e **`INICIO_FAIXA_DETECCAO_CM`** + **`FAIXA_DETECCAO_CM`**, será ignorado.

- **`INTERVALO_AMOSTRAGEM_MOVIMENTO_MS`**: define o delay (em ms) entre cada amostra coletada. Note que não necessariamente será este delay na realidade, mas diminuir ele pode aumentar a velocidade com que o controlador detecta a distância.

- **`CONTAGEM_ITERACOES_CONEXAO_250MS`**: Durante a conexão com o WiFi ou SinricPro, algumas verificações são feitas a cada 250ms antes de reiniciar a ESP32 e tentar outra vez até que conecte. O valor aqui definido, determina quantas vezes essas verificações serão feitas.

- **`TENTATIVAS_ENVIO_EVENTO`**: determina quantas tentativas seguidas, sem sucesso, de envio de evento para o servidor SinricPro podem ser feitas. Caso o valor seja **maior** que isso, a placa reiniciará.

- **`WIFI_SSID`**: o **SSID** da sua rede WiFi.

- **`WIFI_SENHA`**: a **senha** do seu WiFi.

- **`CHAVE_APP`**: a sua *APP_KEY* do SinricPro.

- **`SENHA_APP`**: a sua *APP_SECRET* do SinricPro.

- **`ID_INTERRUPTOR`**: o *SWITCH_ID* do seu dispositivo SinricPro.

- **`VALOR_TIMER_REINICIO_SEG`**: define o valor (em seg) que um timer irá esperar a partir do momento que alguém entrar no corredor até retornar a ESP32 para seu estado original (para caso detecte erroneamente que alguém entrou).

- **`VALOR_TIMER_ESPERA_SEG`**: define o valor (em seg) que um timer irá esperar a partir do momento que for feita uma contagem negativa de pessoas até retornar a ESP32 para seu estado original (isso ocorre quando alguém passa sem ser detectado pelo sensor e sai em seguida, sendo detectado pelo sensor, causando contagem negativa de pessoas).

</p>

## Cuidados a serem tomados

<p align="justify">
  
1. **Verifique se os valores que você utilizou estão de acordo com as capacidades do seu módulo e são suficientes para a execução**: uma stack grande pode desperdiçar espaço ou provocar falhas na alocação, ao passo que uma pequena pode causar estouro de pilha.

2. **Teste os valores relacionados à amostragem**: uma taxa alta pode causar erros que vão desde problemas para notificar o SinricPro até Starvation ( a task principal monopoliza o processador) e uma baixa pode gerar cálculos imprecisos .Uma faixa de detecção grande ou pequena demais pode permitir uma maior chance de erros de detecção, ligando ou desligando o switch quando não deve. Para evitar dores de cabeça, teste bem os valores antes de implantar seu dispositivo definitivamente.

3. **Verifique os arquivos de configuração**: caso altere o **`BAUD_RATE`** no arquivo **main**, altere o **`monitor_speed`** no [platformio.ini](platformio.ini). Veja se o seu módulo é compatível com as configurações que estão citadas lá.

</p>

## Considerações finais

<p align="justify">
Por enquanto, o dispositivo em questão se trata apenas de um partilhamento daquilo que pude desenvolver no projeto. Isto é apenas uma versão funcional, mas ainda ineficiente, que sofrerá algumas alterações em breve; além de contar apenas com um sensor, abrindo margem para alguns erros que não podem (ao menos a meu ver), ser contornados apenas com estes componentes. Com isso em mente, se sinta à vontade para criar sua própria versão, adicionar outros componentes para  suprimir as falhas presentes no projeto.
</p>
