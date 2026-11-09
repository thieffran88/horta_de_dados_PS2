/* Horta de Dados PS2 - textos em portugues (principal), ingles e espanhol. */
#include "lang.h"
#include "game.h"

int g_lang = LANG_PT;

static const char *STR[S_COUNT][NUM_LANGS] = {
    [S_TAGLINE] = { "Role, plante, alimente o porco e venda bem!",
                    "Roll, plant, feed the pig and sell well!",
                    "¡Tira, planta, alimenta al cerdo y vende bien!" },
    [S_MENU_NEW] = { "Novo jogo", "New game", "Nueva partida" },
    [S_MENU_RULES] = { "Como jogar", "How to play", "Cómo jugar" },
    [S_MENU_LANG] = { "Idioma", "Language", "Idioma" },
    [S_LANG_PROMPT] = { "Escolha o idioma", "Choose your language", "Elige el idioma" },
    [S_HINT_CHOOSE] = { "{U}{D} escolher", "{U}{D} choose", "{U}{D} elegir" },
    [S_CONFIRM] = { "Confirmar", "Confirm", "Confirmar" },
    [S_BACK] = { "{O} voltar", "{O} back", "{O} volver" },
    [S_SETUP_TITLE] = { "NOVA PARTIDA", "NEW GAME", "NUEVA PARTIDA" },
    [S_PLAYERS] = { "Jogadores", "Players", "Jugadores" },
    [S_PLAYER_N] = { "Jogador %d", "Player %d", "Jugador %d" },
    [S_MODE] = { "Modo", "Mode", "Modo" },
    [S_BASIC] = { "Básico", "Basic", "Básico" },
    [S_ADVANCED] = { "Avançado", "Advanced", "Avanzado" },
    [S_HUMAN] = { "Humano", "Human", "Humano" },
    [S_CPU_EASY] = { "CPU Fácil", "CPU Easy", "CPU Fácil" },
    [S_CPU_NORMAL] = { "CPU Normal", "CPU Normal", "CPU Normal" },
    [S_START] = { "Começar!", "Start!", "¡Empezar!" },
    [S_BASIC_DESC] = {
        "Básico: cada verdura começa valendo 1 ponto e sobe +1 a cada círculo do mercado.",
        "Basic: each vegetable starts worth 1 point and gains +1 for each market circle.",
        "Básico: cada verdura empieza valiendo 1 punto y sube +1 por cada círculo del mercado." },
    [S_ADV_DESC] = {
        "Avançado: mercados começam em 0, os poderes do porco também trocam a cor do dado e há +5 "
        "pontos para quem tiver mais cenouras, alfaces, tomates e círculos no porco.",
        "Advanced: markets start at 0, pig powers can also change a die's color, and there are +5 "
        "points for the most carrots, lettuces, tomatoes and pig circles.",
        "Avanzado: los mercados empiezan en 0, los poderes del cerdo también cambian el color del dado "
        "y hay +5 puntos para quien tenga más zanahorias, lechugas, tomates y círculos del cerdo." },
    [S_HINT_SETUP] = { "{U}{D} opção   {L}{R} mudar", "{U}{D} option   {L}{R} change",
                       "{U}{D} opción   {L}{R} cambiar" },
    [S_ROUND_MODE] = { "Rodada %d  ·  %s", "Round %d  ·  %s", "Ronda %d  ·  %s" },
    [S_LAST_ROUND] = { "ÚLTIMA RODADA!", "LAST ROUND!", "¡ÚLTIMA RONDA!" },
    [S_TURN] = { "Vez: %s", "Turn: %s", "Turno: %s" },
    [S_WILL_ROLL] = { "%s vai rolar os dados...", "%s is rolling the dice...", "%s va a tirar los dados..." },
    [S_PRESS_ROLL] = { "%s  ·  controle %d  ·  {X} rolar", "%s  ·  controller %d  ·  {X} roll",
                       "%s  ·  mando %d  ·  {X} tirar" },
    [S_MARKET] = { "Mercado", "Market", "Mercado" },
    [S_WORTH] = { "vale %d", "%d pt", "vale %d" },
    [S_CANT_PLANT] = { "Não dá para plantar! {X} alimentar o porco (+%d)", "Can't plant it! {X} feed the pig (+%d)",
                       "¡No se puede plantar! {X} alimentar al cerdo (+%d)" },
    [S_VIEWING] = { "Vendo: horta de %s", "Viewing: %s's garden", "Viendo: huerto de %s" },
    [S_PIG] = { "Porco", "Pig", "Cerdo" },
    [S_POWERS] = { "Poderes: %d", "Powers: %d", "Poderes: %d" },
    [S_USING] = { "usando %d", "using %d", "usando %d" },
    [S_H_ROLL] = { "{X} rolar os dados    {ST} pausa", "{X} roll the dice    {ST} pause",
                   "{X} tirar los dados    {ST} pausa" },
    [S_H_ROLLING] = { "Rolando...", "Rolling...", "Tirando..." },
    [S_H_PICK] = { "{L}{R} dado    {X} pegar    {L1}{R1} ver hortas    {ST} pausa",
                   "{L}{R} die    {X} take    {L1}{R1} view gardens    {ST} pause",
                   "{L}{R} dado    {X} coger    {L1}{R1} ver huertos    {ST} pausa" },
    [S_H_PLACE] = { "{X} confirmar    {O} voltar    {U}{D} linha    {L}{R} poder do porco",
                    "{X} confirm    {O} back    {U}{D} row    {L}{R} pig power",
                    "{X} confirmar    {O} volver    {U}{D} fila    {L}{R} poder del cerdo" },
    [S_H_PLACE_ADV] = { "{X} confirmar   {O} voltar   {U}{D} linha   {L}{R} valor   {T} cor",
                        "{X} confirm   {O} back   {U}{D} row   {L}{R} value   {T} color",
                        "{X} confirmar   {O} volver   {U}{D} fila   {L}{R} valor   {T} color" },
    [S_H_CPU] = { "A CPU está pensando...  (segure {X} para acelerar)", "The CPU is thinking...  (hold {X} to speed up)",
                  "La CPU está pensando...  (mantén {X} para acelerar)" },
    [S_H_MARKET] = { "{X} continuar", "{X} continue", "{X} continuar" },
    [S_YOUR_TURN] = { "SUA VEZ!", "YOUR TURN!", "¡TU TURNO!" },
    [S_PASS_PAD] = { "Use o controle %d", "Use controller %d", "Usa el mando %d" },
    [S_LEFTOVER] = { "Sobrou um dado na reserva:", "One die is left in the pool:", "Queda un dado en la reserva:" },
    [S_MARKET_UP] = { "Mercado de %s sobe! Agora vale %d.", "%s market goes up! Now worth %d.",
                      "¡Sube el mercado de %s! Ahora vale %d." },
    [S_MARKET_FULL] = { "O mercado de %s já está cheio.", "The %s market is already full.",
                        "El mercado de %s ya está lleno." },
    [S_COLOR_ONLY] = { "(só a cor importa)", "(only the color matters)", "(solo importa el color)" },
    [S_MARKET_MSG] = { "Mercado de %s sobe: cada %s vale %d.", "%s market up: each %s is worth %d.",
                       "Sube el mercado de %s: cada %s vale %d." },
    [S_STARTS] = { "%s começa a partida!", "%s starts the game!", "¡%s empieza la partida!" },
    [S_FED] = { "%s alimentou o porco (+%d).", "%s fed the pig (+%d).", "%s alimentó al cerdo (+%d)." },
    [S_FED_POWER] = { "%s alimentou o porco (+%d) e ganhou um poder!", "%s fed the pig (+%d) and earned a power!",
                      "¡%s alimentó al cerdo (+%d) y ganó un poder!" },
    [S_PLANTED] = { "%s plantou %s na coluna %d.", "%s planted %s in column %d.", "%s plantó %s en la columna %d." },
    [S_TRIGGER] = { "%s ativou o fim de jogo! Esta é a última rodada.", "%s triggered the end! This is the last round.",
                    "¡%s activó el final! Esta es la última ronda." },
    [S_INVALID] = { "Jogada inválida.", "Invalid move.", "Jugada inválida." },
    [S_NO_POWERS_ENOUGH] = { "Sem poderes do porco suficientes!", "Not enough pig powers!",
                             "¡No tienes suficientes poderes del cerdo!" },
    [S_NO_POWERS] = { "Você não tem poderes do porco. Complete uma linha do porco!",
                      "You have no pig powers. Complete a pig row!",
                      "No tienes poderes del cerdo. ¡Completa una fila del cerdo!" },
    [S_COLOR_ADV_ONLY] = { "Trocar a cor só no modo avançado.", "Changing color is only in advanced mode.",
                           "Cambiar el color solo en modo avanzado." },
    [S_NO_POWER_COLOR] = { "Sem poderes do porco para trocar a cor!", "No pig powers to change the color!",
                           "¡Sin poderes del cerdo para cambiar el color!" },
    [S_ONLY_FEED] = { "Só dá para alimentar o porco com um dado que não pode ser plantado.",
                      "You can only feed the pig a die that can't be planted.",
                      "Solo puedes alimentar al cerdo con un dado que no se pueda plantar." },
    [S_PAUSE] = { "PAUSA", "PAUSE", "PAUSA" },
    [S_CONTINUE] = { "Continuar", "Continue", "Continuar" },
    [S_QUIT] = { "Sair para o título", "Quit to title", "Salir al título" },
    [S_GAME_OVER] = { "FIM DE JOGO", "GAME OVER", "FIN DEL JUEGO" },
    [S_CARROTS] = { "Cenouras", "Carrots", "Zanahorias" },
    [S_LETTUCES] = { "Alfaces", "Lettuces", "Lechugas" },
    [S_TOMATOES] = { "Tomates", "Tomatoes", "Tomates" },
    [S_RIBBONS] = { "Laços", "Ribbons", "Lazos" },
    [S_PIG_ROW] = { "Porco", "Pig", "Cerdo" },
    [S_BONUS] = { "Bônus", "Bonus", "Bonus" },
    [S_TOTAL] = { "TOTAL", "TOTAL", "TOTAL" },
    [S_WON] = { "%s venceu!", "%s wins!", "¡%s gana!" },
    [S_TIE] = { "Empate! Vitória dividida: ", "Tie! Shared victory: ", "¡Empate! Victoria compartida: " },
    [S_AND] = { " e ", " and ", " y " },
    [S_BACK_TITLE] = { "{X} voltar ao título", "{X} back to title", "{X} volver al título" },
    [S_PAGE] = { "Página %d/%d", "Page %d/%d", "Página %d/%d" },
    [S_RULES_HINT] = { "{L}{R} páginas    {O} voltar", "{L}{R} pages    {O} back", "{L}{R} páginas    {O} volver" },
    [S_PTS] = { "pts", "pts", "pts" },
};

static const char *LANG_NAMES[NUM_LANGS] = { "Português", "English", "Español" };

static const char *VEG_OBJ[NUM_LANGS][4] = {
    { "", "uma cenoura", "uma alface", "um tomate" },
    { "", "a carrot", "a lettuce", "a tomato" },
    { "", "una zanahoria", "una lechuga", "un tomate" },
};
static const char *VEG_PLURAL[NUM_LANGS][4] = {
    { "", "cenouras", "alfaces", "tomates" },
    { "", "carrots", "lettuces", "tomatoes" },
    { "", "zanahorias", "lechugas", "tomates" },
};
static const char *VEG_SING[NUM_LANGS][4] = {
    { "", "cenoura", "alface", "tomate" },
    { "", "carrot", "lettuce", "tomato" },
    { "", "zanahoria", "lechuga", "tomate" },
};
static const char *VEG_MARKET[NUM_LANGS][4] = {
    { "", "Cenoura", "Alface", "Tomate" },
    { "", "Carrot", "Lettuce", "Tomato" },
    { "", "zanahoria", "lechuga", "tomate" },
};
static const char *CPU_NAMES[NUM_LANGS][MAX_PLAYERS] = {
    { "Seu Zé", "Vó Cida", "Tio Beto", "Dona Lia" },
    { "Farmer Joe", "Granny Sue", "Uncle Bob", "Aunt Lia" },
    { "Don Pepe", "Abuela Rosa", "Tío Beto", "Doña Lía" },
};

static const RulesPage RULES[NUM_LANGS][RULES_PAGES] = {
    { /* ---------------- Portugues ---------------- */
      { "OBJETIVO",
        "Em Horta de Dados você escolhe dados para plantar cenouras, alfaces e tomates na sua "
        "horta ou para engordar o seu porco.\n\n"
        "O preço de cada verdura muda ao longo da partida no mercado. Quem tiver mais pontos "
        "quando a colheita chegar vence!\n\n"
        "Dado laranja = cenoura\nDado verde = alface\nDado vermelho = tomate" },
      { "A RODADA",
        "O jogador inicial (marcado com o porquinho no placar) rola todos os dados: 9 dados, "
        "ou 6 dados numa partida de 2 jogadores.\n\n"
        "Começando por ele e seguindo a ordem dos jogadores, cada um pega 1 dado da reserva, "
        "até sobrar só 1 dado. Com 2 ou 3 jogadores, alguém acaba pegando um dado a menos.\n\n"
        "Todo dado escolhido precisa ser plantado na horta ou dado ao porco." },
      { "PLANTAR",
        "O número do dado indica a coluna da horta (veja as placas acima das colunas). Plante a "
        "verdura da cor do dado em um espaço livre dessa coluna.\n\n"
        "A primeira verdura de cada tipo pode ir em qualquer espaço livre da coluna. Depois "
        "disso, cada nova verdura daquele tipo precisa ficar encostada (em cima, embaixo, à "
        "esquerda ou à direita) em outra verdura igual.\n\n"
        "Se a coluna estiver cheia, ou se não houver vizinho igual, o dado não pode ser plantado." },
      { "ALIMENTAR O PORCO",
        "Um dado que não pode ser plantado vai obrigatoriamente para o porco: marque tantos "
        "círculos quanto o valor do dado, da esquerda para a direita e de cima para baixo.\n\n"
        "Você só pode alimentar o porco com um dado que não cabe na horta. Quer engordar o "
        "porco? Escolha um dado que não dá para plantar!\n\n"
        "No fim, o porco vale os pontos ao lado da última linha completa (2, 4, 6, 8, 10, 12 ou 15)." },
      { "PODERES DO PORCO",
        "Cada linha de 6 círculos completa no porco libera um poder (caixinha *). Cada poder "
        "pode ser usado uma vez.\n\n"
        "Um poder muda o valor do dado escolhido em +1 ou -1 (nunca abaixo de 1 nem acima de 6). "
        "Você pode gastar vários poderes no mesmo dado: dois poderes transformam um 4 em 6.\n\n"
        "Modo avançado: cada poder pode, em vez disso, trocar a cor do dado." },
      { "O MERCADO",
        "Quando sobra só 1 dado na reserva, ninguém o pega. O mercado da cor desse dado sobe um "
        "círculo para todos os jogadores (o número do dado não importa).\n\n"
        "Modo básico: cada verdura vale 1 ponto + 1 por círculo marcado no seu mercado.\n"
        "Modo avançado: os mercados começam vazios, valendo 0.\n\n"
        "Depois disso o próximo jogador vira o inicial e rola os dados de novo." },
      { "FIM E PONTOS",
        "A última rodada começa quando alguém enche a horta (18 espaços), marca todos os 42 "
        "círculos do porco, ou quando um mercado chega a 6 círculos. A rodada termina e então:\n\n"
        "Verduras: quantidade de cada tipo x valor no mercado.\n"
        "Laços: 5 pontos por linha completa da horta.\n"
        "Porco: o valor da última linha completa.\n"
        "Avançado: +5 para quem tem mais cenouras, mais alfaces, mais tomates e mais círculos no "
        "porco (empates: todos ganham).\n\n"
        "Desempate: mais verduras plantadas; depois mais círculos no porco." },
      { "CONTROLES",
        "{L}{R}  escolher um dado da reserva (ou o analógico {AN}).\n"
        "{X}  pegar o dado / plantar / alimentar o porco.\n"
        "Com o dado na mão:\n"
        "      {U}{D}  escolher a linha da coluna.\n"
        "      {L}{R}  gastar um poder do porco (-1 / +1).\n"
        "      {T}  gastar um poder para trocar a cor (avançado).\n"
        "      {O}  devolver o dado e escolher outro.\n"
        "{L1}{R1}{L2}{R2}{SE}  ver a horta dos outros jogadores.\n"
        "{ST}  pausa.      Segure {X} para acelerar a CPU.\n\n"
        "Controle 1: jogador 1. Controle 2: jogadores 2, 3 e 4 (passem o controle 2 entre si)." } },
    { /* ---------------- English ---------------- */
      { "GOAL",
        "In Horta de Dados you pick dice to plant carrots, lettuces and tomatoes in your garden "
        "or to fatten up your pig.\n\n"
        "The price of each vegetable changes during the game at the market. Whoever has the "
        "most points when harvest time comes wins!\n\n"
        "Orange die = carrot\nGreen die = lettuce\nRed die = tomato" },
      { "THE ROUND",
        "The first player (marked with the piglet on the scoreboard) rolls all the dice: 9 dice, "
        "or 6 dice in a 2-player game.\n\n"
        "Starting with them and going in player order, everyone takes 1 die from the pool until "
        "only 1 die is left. With 2 or 3 players, someone ends up taking one die less.\n\n"
        "Every die you take must be planted in your garden or fed to your pig." },
      { "PLANTING",
        "The die's number is the garden column (see the signs above the columns). Plant the "
        "vegetable of the die's color in a free space of that column.\n\n"
        "The first vegetable of each kind can go in any free space of the column. After that, "
        "each new vegetable of that kind must touch (above, below, left or right) another "
        "vegetable of the same kind.\n\n"
        "If the column is full, or there is no matching neighbor, the die can't be planted." },
      { "FEEDING THE PIG",
        "A die that can't be planted must go to the pig: mark as many circles as the die's "
        "value, from left to right and top to bottom.\n\n"
        "You may only feed the pig a die that doesn't fit in your garden. Want a fatter pig? "
        "Pick a die you can't plant!\n\n"
        "At the end, the pig scores the value next to its last completed row (2, 4, 6, 8, 10, 12 or 15)." },
      { "PIG POWERS",
        "Each completed row of 6 circles on the pig unlocks a power (the * box). Each power can "
        "be used once.\n\n"
        "A power changes the chosen die's value by +1 or -1 (never below 1 or above 6). You can "
        "spend several powers on the same die: two powers turn a 4 into a 6.\n\n"
        "Advanced mode: a power can instead change the die's color." },
      { "THE MARKET",
        "When only 1 die is left in the pool, nobody takes it. The market of that die's color "
        "goes up one circle for every player (the number doesn't matter).\n\n"
        "Basic mode: each vegetable is worth 1 point + 1 per marked circle in its market.\n"
        "Advanced mode: markets start empty, worth 0.\n\n"
        "Then the next player becomes the first player and rolls the dice again." },
      { "END AND SCORING",
        "The last round starts when someone fills their garden (18 spaces), marks all 42 pig "
        "circles, or when a market reaches 6 circles. The round is finished and then:\n\n"
        "Vegetables: amount of each kind x its market value.\n"
        "Ribbons: 5 points per complete garden row.\n"
        "Pig: the value of its last completed row.\n"
        "Advanced: +5 for the most carrots, most lettuces, most tomatoes and most pig circles "
        "(ties: everyone tied gets it).\n\n"
        "Tiebreak: most vegetables planted; then most pig circles." },
      { "CONTROLS",
        "{L}{R}  choose a die from the pool (or the analog stick {AN}).\n"
        "{X}  take the die / plant / feed the pig.\n"
        "With a die in hand:\n"
        "      {U}{D}  choose the row in the column.\n"
        "      {L}{R}  spend a pig power (-1 / +1).\n"
        "      {T}  spend a power to change color (advanced).\n"
        "      {O}  put the die back and pick another.\n"
        "{L1}{R1}{L2}{R2}{SE}  look at the other players' gardens.\n"
        "{ST}  pause.      Hold {X} to speed up the CPU.\n\n"
        "Controller 1: player 1. Controller 2: players 2, 3 and 4 (pass controller 2 around)." } },
    { /* ---------------- Espanol ---------------- */
      { "OBJETIVO",
        "En Horta de Dados eliges dados para plantar zanahorias, lechugas y tomates en tu huerto "
        "o para engordar a tu cerdo.\n\n"
        "El precio de cada verdura cambia durante la partida en el mercado. ¡Quien tenga más "
        "puntos cuando llegue la cosecha gana!\n\n"
        "Dado naranja = zanahoria\nDado verde = lechuga\nDado rojo = tomate" },
      { "LA RONDA",
        "El jugador inicial (marcado con el cerdito en el marcador) tira todos los dados: 9 "
        "dados, o 6 dados en una partida de 2 jugadores.\n\n"
        "Empezando por él y siguiendo el orden de los jugadores, cada uno coge 1 dado de la "
        "reserva hasta que solo quede 1. Con 2 o 3 jugadores, alguien coge un dado menos.\n\n"
        "Cada dado elegido debe plantarse en el huerto o dárselo al cerdo." },
      { "PLANTAR",
        "El número del dado indica la columna del huerto (mira los carteles sobre las columnas). "
        "Planta la verdura del color del dado en un espacio libre de esa columna.\n\n"
        "La primera verdura de cada tipo puede ir en cualquier espacio libre de la columna. "
        "Después, cada nueva verdura de ese tipo debe estar junto (arriba, abajo, a la izquierda "
        "o a la derecha) a otra verdura igual.\n\n"
        "Si la columna está llena, o no hay vecino igual, el dado no se puede plantar." },
      { "ALIMENTAR AL CERDO",
        "Un dado que no se puede plantar va obligatoriamente al cerdo: tacha tantos círculos "
        "como el valor del dado, de izquierda a derecha y de arriba abajo.\n\n"
        "Solo puedes alimentar al cerdo con un dado que no cabe en el huerto. ¿Quieres engordar "
        "al cerdo? ¡Elige un dado que no se pueda plantar!\n\n"
        "Al final, el cerdo vale los puntos junto a su última fila completa (2, 4, 6, 8, 10, 12 o 15)." },
      { "PODERES DEL CERDO",
        "Cada fila de 6 círculos completa del cerdo desbloquea un poder (casilla *). Cada poder "
        "se usa una sola vez.\n\n"
        "Un poder cambia el valor del dado elegido en +1 o -1 (nunca menos de 1 ni más de 6). "
        "Puedes gastar varios poderes en el mismo dado: dos poderes convierten un 4 en un 6.\n\n"
        "Modo avanzado: un poder también puede cambiar el color del dado." },
      { "EL MERCADO",
        "Cuando solo queda 1 dado en la reserva, nadie lo coge. El mercado del color de ese dado "
        "sube un círculo para todos los jugadores (el número no importa).\n\n"
        "Modo básico: cada verdura vale 1 punto + 1 por círculo tachado en su mercado.\n"
        "Modo avanzado: los mercados empiezan vacíos, valiendo 0.\n\n"
        "Después, el siguiente jugador pasa a ser el inicial y vuelve a tirar los dados." },
      { "FIN Y PUNTOS",
        "La última ronda empieza cuando alguien llena su huerto (18 espacios), tacha los 42 "
        "círculos del cerdo, o cuando un mercado llega a 6 círculos. Se termina la ronda y:\n\n"
        "Verduras: cantidad de cada tipo x su valor de mercado.\n"
        "Lazos: 5 puntos por fila completa del huerto.\n"
        "Cerdo: el valor de su última fila completa.\n"
        "Avanzado: +5 para quien tenga más zanahorias, más lechugas, más tomates y más círculos "
        "del cerdo (empates: todos lo reciben).\n\n"
        "Desempate: más verduras plantadas; después más círculos del cerdo." },
      { "CONTROLES",
        "{L}{R}  elegir un dado de la reserva (o el stick analógico {AN}).\n"
        "{X}  coger el dado / plantar / alimentar al cerdo.\n"
        "Con el dado en la mano:\n"
        "      {U}{D}  elegir la fila de la columna.\n"
        "      {L}{R}  gastar un poder del cerdo (-1 / +1).\n"
        "      {T}  gastar un poder para cambiar el color (avanzado).\n"
        "      {O}  devolver el dado y elegir otro.\n"
        "{L1}{R1}{L2}{R2}{SE}  ver los huertos de los demás.\n"
        "{ST}  pausa.      Mantén {X} para acelerar a la CPU.\n\n"
        "Mando 1: jugador 1. Mando 2: jugadores 2, 3 y 4 (pasaos el mando 2)." } },
};

static int L(void) { return (g_lang >= 0 && g_lang < NUM_LANGS) ? g_lang : 0; }

const char *T(int id) { return (id >= 0 && id < S_COUNT && STR[id][L()]) ? STR[id][L()] : "?"; }
const char *lang_name(int lang) { return LANG_NAMES[lang]; }
const char *veg_obj(int veg) { return VEG_OBJ[L()][veg & 3]; }
const char *veg_plural(int veg) { return VEG_PLURAL[L()][veg & 3]; }
const char *veg_sing(int veg) { return VEG_SING[L()][veg & 3]; }
const char *veg_market(int veg) { return VEG_MARKET[L()][veg & 3]; }
const char *cpu_name(int i) { return CPU_NAMES[L()][i & 3]; }
const RulesPage *rules_page(int page) { return &RULES[L()][page]; }
