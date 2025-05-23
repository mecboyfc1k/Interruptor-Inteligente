#include <Arduino.h>
#include <WiFi.h>
#include <SinricPro.h>
#include <SinricProSwitch.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HTTPClient.h>

#define BAUD_RATE 9600
#define TAMANHO_STACK_TASK_PRINCIPAL 8192
#define TAMANHO_STACK_VERIFICADORES_CONEXAO 2048
#define TRIG 32
#define ECHO 33
#define FAIXA_DETECCAO_CM 20
#define INICIO_FAIXA_DETECCAO_CM 10
#define INTERVALO_AMOSTRAGEM_MOVIMENTO_MS 100
#define TIMER_REINICIO 0
#define TIMER_ESPERA 1
#define VALOR_PRESCALER 80
#define CONTAGEM_ITERACOES_CONEXAO_250MS 50 //Quantas iterações de 250ms o código irá fazer antes de desistir da conexão com WiFi ou SinricPro. Exemplo: 50 tentativas resultam em 250 * 50 = 12500 ms ou 12,5 seg antes de tentar conectar outra vez.
#define TENTATIVAS_ENVIO_EVENTO 5 //Número máximo de tentativas consecutivas de envio de eventos para o app antes de reiniciar.
//Informações sobre WiFi e dispositivo do SinricPro

#define WIFI_SSID ""
#define WIFI_SENHA ""

  //Foge ao escopo desta documentação explicar como conseguir os dados seguintes. Para isso, consulte https://help.sinric.pro/pages/tutorials/switch/part-1

#define CHAVE_APP ""
#define SENHA_APP ""
#define ID_INTERRUPTOR ""
//

const uint32_t TAXA_FREQUENCIA_TIMERS = 80000000 / VALOR_PRESCALER;
const uint32_t VALOR_TIMER_REINICIO_SEG = 300; //Tempo de espera (em segundos) para reiniciar o sistema após um número negativo de pessoas ser contado. Suporta até cerca de 1h11min.
const uint32_t VALOR_TIMER_ESPERA_SEG = 1200; //Tempo de espera (em segundos) após uma nova pessoa entrar no local. Suporta até cerca de 1h11min.

float volatile leitura_anterior = 0;
float volatile variacao_total = 0;
int8_t volatile contagem_de_pessoas = 0;
int8_t volatile contagem_de_pessoas_ant = 0;
hw_timer_t* timer_reinicio = NULL;
hw_timer_t* timer_espera = NULL;
SinricProSwitch& meu_switch = SinricPro[ID_INTERRUPTOR];
bool volatile resposta_app = true; //Serve para verificar se o envio de um evento para o app foi bem sucedido
uint8_t volatile num_de_tentativas_eventos = 0; //Faz a contagem de tentativas falhas de notificação de evetos

//Variáveis utilizadas para as tasks que serão rodadas

TaskHandle_t verificador_wifi = NULL, verificador_sinric = NULL, principal = NULL; 
StaticTask_t xBufferTaskPrincipal;
StackType_t xStackTaskPrincipal[TAMANHO_STACK_TASK_PRINCIPAL];

//Verifica se, apesar da conexão WiFi estabelecida, há conexão com a internet.
bool internet_acessivel(){

  //OBS: Caso deseje maior rapidez, crie uma conexão persistente ou utilize uma outra Biblioteca mais adequada. Como na minha aplicação não foi necessário, deixei assim mesmo por questão de simplicidade.

  HTTPClient http;
  http.begin("http://www.google.com");
  int codigo_http = http.GET();

  http.end();

  if(codigo_http > 0)
    return true;
  
  return false;
}

//Basicamente uma resposta aos problemas de conexão com o WiFi, que reinicia o programa que está rodando no módulo.
void verificar_wifi(void * pvParameters)
{
  while(true){

    if(WiFi.status() == WL_DISCONNECTED){
      vTaskDelete(verificador_sinric);
      vTaskDelete(principal);
      Serial.print("[WiFi]Voce foi desconectado!\r\n");
      Serial.print("[WiFi]Preparando para reiniciar\r\n");
      for(int i = 0; i < 3; i++){
        delay(250);
        Serial.print(".");
      }
      delay(250);
      ESP.restart();
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}
//

//Basicamente uma resposta aos problemas de conexão com o SinricPro, que reinicia o programa que está rodando no módulo.
void verificar_sinric(void * pvParameters){

  while(true){

    if(!internet_acessivel() || num_de_tentativas_eventos > TENTATIVAS_ENVIO_EVENTO){
      vTaskDelete(verificador_wifi);
      vTaskDelete(principal);
      Serial.print("[Sinric]Voce foi desconectado!\r\n");
      Serial.print("[Sinric]Preparando para reiniciar\r\n");
      for(int i = 0; i < 3; i++){
        delay(250);
        Serial.print(".");
      }
      delay(250);
      ESP.restart();
    }
  vTaskDelay(pdMS_TO_TICKS(250));
  }
}
//

//(Comentário para melhor legibilidade)
void conectar_wifi(){

  uint8_t contador = 0;
  Serial.print("\r\n[WiFi]Conectando");

  WiFi.begin(WIFI_SSID, WIFI_SENHA);
  WiFi.setSleep(false);

  do
  {
    Serial.print(".");
    digitalWrite(BUILTIN_LED, HIGH);
    delay(125);
    digitalWrite(BUILTIN_LED, LOW);
    delay(125);
    contador ++;
    if(contador == CONTAGEM_ITERACOES_CONEXAO_250MS){
      Serial.print("\r\n[WiFi]Conexao falhou. Tentando outra vez...\r\n");
      delay(250);
      ESP.restart();
    }
  }while (WiFi.status() != WL_CONNECTED);
  digitalWrite(BUILTIN_LED, LOW);

  Serial.printf("conectado!\r\n[WiFi]: o IP eh %s\r\n", WiFi.localIP().toString().c_str());
  if(!internet_acessivel()){
    Serial.print("\r\n[WiFi]Conexao com a internet falhou. Tentando outra vez...\r\n");
    delay(250);
    ESP.restart();
  }

}
//

//(Comentário para melhor legibilidade)
void conectar_switch(){

  uint8_t contador = 0;

  SinricPro.onConnected([](){ Serial.printf("conectado!!\r\n"); }); 
  
  SinricPro.begin(CHAVE_APP, SENHA_APP);
  Serial.print("\r\n[SinricPro]Conectando");
  
  do
  {
    
    if(contador == CONTAGEM_ITERACOES_CONEXAO_250MS || !internet_acessivel()){
      Serial.print("\r\n[SinrcicPro]Conexao falhou. Tentando outra vez...\r\n");
      delay(250);
      ESP.restart();
    }
    Serial.print(".");
    SinricPro.handle();
    digitalWrite(BUILTIN_LED, HIGH);
    delay(125);
    digitalWrite(BUILTIN_LED, LOW);

    contador ++;

  }while (!SinricPro.isConnected());

  digitalWrite(BUILTIN_LED, LOW);
}
//


//Esta função servirá para retornar o sistema ao estado original quando o limite dos timers de espera e de reinício forem atingidos. Em outras palavras, caso o sistema apresente comportamento indevido previsto, um timer será ativado para que o sistema reinicie, chamando esta função.
void IRAM_ATTR on_limite_tempo_atingido(){

  contagem_de_pessoas = 0;
  if(contagem_de_pessoas_ant < contagem_de_pessoas)
    contagem_de_pessoas_ant = 1;
  Serial.print("O tempo esgotou!\r\n");
}
//

void task_principal(void * pvParameters){

  while(true){

    if(contagem_de_pessoas > 0 && contagem_de_pessoas != contagem_de_pessoas_ant){

      //Quando a contagem de pessoas sair de 0, a lâmpada será ligada.
      if(SinricPro.isConnected())
        resposta_app = meu_switch.sendPowerStateEvent(true);

      //Se o alarme do timer de espera estiver desativado, será ativado e a contagem de pessoas anterior será igualada à contagem atual, evitando que o alarme seja reiniciado na próxima iteração.
      if(!timerAlarmEnabled(timer_espera)){
        timerRestart(timer_espera);
        timerAlarmEnable(timer_espera); //Pode-se perguntar a finalidade deste alarme. Em casos em que uma pessoa saia rápido demais do local ou se movimente de forma irregular na área de detecção, e não entre no local, pode ser que a contagem seja aumentada, mesmo que na realidade esteja vazio. Assim, é dado um tempo limite para o qual uma pessoa permanecerá no local.

        Serial.print("Alarme iniciado!\r\n");
        Serial.print("Uma pessoa entrou!\r\n");
        contagem_de_pessoas_ant = contagem_de_pessoas;

      //O contador irá reiniciar todas as vezes que alguém entrar, garantindo que independente da ordem de entrada, todos tenham o mesmo tempo.
      }else{
        if(contagem_de_pessoas > contagem_de_pessoas_ant){
          contagem_de_pessoas_ant = contagem_de_pessoas;
          timerRestart(timer_espera);
          Serial.print("Timer reiniciado!\r\n");
          Serial.print("Uma pessoa entrou!\r\n");
        }

        else if(contagem_de_pessoas < contagem_de_pessoas_ant)
          Serial.print("Uma pessoa saiu!\r\n");
          contagem_de_pessoas_ant = contagem_de_pessoas;
      }
    }

    //Se a contagem zerar, seja pelo alarme ou seja por todas as pessoas terem saído do local, e havia alguém anteriormente, pelo menos para o programa, a lâmpada será apagada e as contagens serão igualadas, além de desativar o alarme caso esteja ativado.

    if(contagem_de_pessoas == 0 && contagem_de_pessoas < contagem_de_pessoas_ant){
      if(SinricPro.isConnected())
        resposta_app = meu_switch.sendPowerStateEvent(false);
      contagem_de_pessoas_ant = contagem_de_pessoas;
      if(timerAlarmEnabled(timer_espera))
        timerAlarmDisable(timer_espera);
      Serial.print("O local voltou a estar vazio... tao solitario :`(\r\n");
    }
    //No caso de uma pessoa passar extremamente rápido pelo sensor ao entrar, ou se mover de forma irregular e o sensor não detecte entrada, ao sair, o sensor pode contabilizar um número negativo de pessoas. O microcontrolador dará um tempo de carência para caso haja mais alguma pessoa que não foi dectada. Caso uma pessoa entre no tempo de carência (o que pode mudar a contagem para 0 caso apenas -1 pessoa esteja sendo contada, mas não mudar o fato de que havia -1 pessoa anteriormente), será contado como tendo exatamente uma pessoa no local.
    if(contagem_de_pessoas < 0 || contagem_de_pessoas_ant < 0){
      if(contagem_de_pessoas_ant > contagem_de_pessoas){

        if(!timerAlarmEnabled(timer_reinicio)){
          timerRestart(timer_reinicio);
          timerAlarmEnable(timer_reinicio);
          if(SinricPro.isConnected())
            resposta_app = meu_switch.sendPowerStateEvent(true);
        }else
          timerRestart(timer_reinicio);

        Serial.print("Menos uma pessoa???\r\n");
        contagem_de_pessoas_ant = contagem_de_pessoas;

      }else if(contagem_de_pessoas_ant < contagem_de_pessoas){
        contagem_de_pessoas = 1;
        timerAlarmDisable(timer_reinicio);
      }
    }

    SinricPro.handle();

    //Define um limite de tentativas de comunicação com o SinricPro para notificação de eventos. Geralmente isso ocorre se houver uma taxa de notificação muito grande, o que é definido pela quantidade de vezes que a contagem de pessoas é atualizada.
    if(!resposta_app){
      Serial.print("[SinricPro]Falha ao tentar atualizar estado\r\n");
      num_de_tentativas_eventos++;
      resposta_app = true;
    }else if(num_de_tentativas_eventos)
      num_de_tentativas_eventos = 0;

      //Detecção da distância até o objeto mais próximo
    digitalWrite(TRIG, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    
    long duracao = pulseIn(ECHO, HIGH);
    float distancia = duracao * 0.034 / 2;
    //

    //Filtragem: caso o valor esteja fora da faixa de detecção será ignorado. Veja que para casos de interferências ou não retorno do sinal (casos que retornam cerca de 798cm ou 0cm), o valor será ignorado, desde que o início da faixa de detecção seja diferente de 0.

    if(!(distancia < INICIO_FAIXA_DETECCAO_CM || distancia > INICIO_FAIXA_DETECCAO_CM + FAIXA_DETECCAO_CM)){

      //Na primeira variação dentro da faixa de deteccao, só uma atualização das variáveis globais é feita para a variação total e a leitura anterior

      if(leitura_anterior == 0)
        leitura_anterior = distancia;

      else{

        variacao_total += leitura_anterior - distancia; //Note que caso a pessoa se aproxime do sensor, a variação tenderá a ser positiva. O contrário também é verdade.

        leitura_anterior = distancia;

      }

      vTaskDelay(pdMS_TO_TICKS(INTERVALO_AMOSTRAGEM_MOVIMENTO_MS));

    }else{

      //Quando alguém sai da faixa de detecção, inevitavelmente a leitura anterior será diferente de 0, a menos que o início dela seja definido para 0 e de alguma forma 0 seja lido.

      if(leitura_anterior != 0){
        if(variacao_total > 0)
          contagem_de_pessoas += 1;

        else if(variacao_total < 0)
          contagem_de_pessoas -= 1;

        leitura_anterior = 0;
        variacao_total = 0;
      }

      //OBS: Por favor, evite utilizar 0 como parte da faixa de detecção, pois ele é usado para detecção de comportamentos anormais no envio e recebimento dos sinais do sensor, assim como valores acima de 790cm. Note que qualquer valor fora dessa faixa "aceitável" dispara precocemente o próximo envio de sinal.

      if(distancia > 790 || distancia == 0){
        vTaskDelay(pdMS_TO_TICKS(50));
        continue;
      }
      else
        vTaskDelay(pdMS_TO_TICKS(INTERVALO_AMOSTRAGEM_MOVIMENTO_MS));
    }
  }
}


void setup() {
  
  Serial.begin(BAUD_RATE);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);

  //Aqui são criados dois timers diferentes ligados à mesma função, cada um disparado em momentos diferentes com durações diferentes. Um para o caso de uma contagem positiva quando houverem 0 pessoas (timer_espera) e outro para contagem negativa de pessoas (timer_reinicio), que acontece na não detecção de uma pessoa ao entrar no corredor e sair sendo detectada.

  timer_espera = timerBegin(TIMER_ESPERA, VALOR_PRESCALER, true);
  timer_reinicio = timerBegin(TIMER_REINICIO, VALOR_PRESCALER, true);

  if (timer_espera == NULL || timer_reinicio == NULL) {
    Serial.print("Erro ao criar timers!\r\n");
    ESP.restart();
  }

  timerAttachInterrupt(timer_espera, on_limite_tempo_atingido, true);
  timerAttachInterrupt(timer_reinicio, on_limite_tempo_atingido, true);

  timerAlarmWrite(timer_espera, VALOR_TIMER_ESPERA_SEG * TAXA_FREQUENCIA_TIMERS, false);
  timerAlarmWrite(timer_reinicio, VALOR_TIMER_REINICIO_SEG * TAXA_FREQUENCIA_TIMERS, false);
  //


  //Conexão com WiFi, SinricPro e criação das tasks.
  conectar_wifi();
  conectar_switch();
  if(SinricPro.isConnected())
    meu_switch.sendPowerStateEvent(false);
  SinricPro.handle();
  principal = xTaskCreateStatic(task_principal, "task_principal", TAMANHO_STACK_TASK_PRINCIPAL, NULL, 5, xStackTaskPrincipal, &xBufferTaskPrincipal);
  xTaskCreate(verificar_wifi, "verificar_wifi", TAMANHO_STACK_VERIFICADORES_CONEXAO, NULL, 1, &verificador_wifi);
  xTaskCreate(verificar_sinric, "verificar_sinric", TAMANHO_STACK_VERIFICADORES_CONEXAO, NULL, 1, &verificador_sinric);
  
  //

  Serial.print("Tudo pronto!\r\n");

}


void loop(){}