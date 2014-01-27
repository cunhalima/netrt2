netrt2
======

Este programa faz o roteamento de pacotes UDP de modo confiável, e descobre a topologia de forma dinâmica.

1 Execução
----------
A sintaxe da linha de comando é a seguinte:

    ./net <id-do-roteador> [ -e percentual_de_erro_simulado ]

Assim, cada nó (roteador) corresponde a um processo (uma instância do programa "net").
Ao executar o programa, será exibido na tela a mensagem "node number = <n>" onde
<n> é o id do nó fornecido na linha de comando.
Em seguida, o programa aguardará que o usuário escreva um comando ou envie uma mensagem de bate-papo na entrada padrão (teclado).
Há os seguintes comandos (todos se iniciam com barra "/"):

    Comando                       | Descrição
    ------------------------------+-----------
    /tabs ou /t                   | mostra as principais tabelas do roteador na tela
    /showtime <milissegundos>     | altera o intervalo de tempo da impressão automática das tabelas na tela
    /ttl <ttl>                    | altera o time-to-live dos pacotes roteados
    /dropadd <0/1>                | esconde/mostra a depuração da queda/adição de roteadores
    /debugrt <0/1>                | esconde/mostra a depuração da tabela de roteamento
    /debugpacket <0/1>            | esconde/mostra a depuração dos pacotes enviados/recebidos
    /debuglost <0/1>              | esconde/mostra a depuração dos pacotes perdidos
    /debugroute <0/1>             | esconde/mostra as mensagens de roteamento
    /drop <0/1>                   | força a queda de um vizinho (a partir do ponto de vista deste nó)
    /stall                        | coloca o algoritmo no modo "tempo congelado" ou "parado" (para fins de depuração)
    /hb                           | força um heartbeat (envio de tabela vertor-distância aos vizinhos)
    /hc                           | força um heartcheck (verificação de vizinhos desligados)
    /sleep                        | simula a queda do presente nó
    /wake                         | termina a simulação de queda do presente nó
    /all <comando ...>            | envia um comando para todos os nós (inclusive para o nó atual) -- para fins de depuração
    /hbtime <milissegundos>       | altera o intervalo de tempo entre heartbeats consecutivos
    /hctime <milissegundos>       | altera o intervalo de tempo entre heartchecks consecutivos
    /rtimeout <milissegundos>     | altera o intervalo de tempo entre tentativas de entrega de mensagem confiável
    /rattempts <tentativas>       | altera a quantidade máxima de tentativas de entrega de mensagem confiável
    /infinity <custo>             | altera o custo considerado infinito (para prevenção de contagem ao infinito)
    /quit                         | termina o programa

Além disso, para enviar uma mensagem a um nó, basta digitar o número do nó de destino seguido da mensagem:

    <destino> <mensagem> [ENTER]

Para encerrar o programa, basta teclar <ESC> ou digitar o comando "/quit".

2 Saídas na tela
----------------

Durante a execução do programa, são mostradas algumas saídas na tela.
Mensagens de depuração se iniciam com a marca "[DEBUG]".
Também são mostradas mensagens de envio e recebimento de mensagens de bate-papo.
