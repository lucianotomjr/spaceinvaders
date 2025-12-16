/*
    Space Invaders
    Autores: João Matheus Cachoeira Pimentel, Luciano Tomasi Junior e Luiza Almeida Deon
*/

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <list>
#include <cmath>
#include <fstream>
#include <ctime>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include <set>

using namespace std;

// Constantes Gráficas
const float TAM_CELULA = 25.0f;
const int LARGURA_GRID = 40;
const int ALTURA_GRID = 25;
const float ALTURA_HUD = 100.0f;
const float DT_FIXO = 1.0f / 60.0f;

const unsigned int LARGURA_JANELA = static_cast<unsigned int>(LARGURA_GRID * TAM_CELULA);
const unsigned int ALTURA_JANELA = static_cast<unsigned int>((ALTURA_GRID * TAM_CELULA) + ALTURA_HUD);

// Estados do Jogo
enum EstadoJogo {
    MENU_PRINCIPAL,
    TELA_AJUDA,
    MENU_JOGADORES,
    MENU_MODO_JOGO,
    MENU_MODO_NPC,
    MENU_DIFICULDADE,
    SELECAO_NAVE,
    DIGITAR_NOME_P1,
    DIGITAR_NOME_P2,
    JOGANDO,
    PAUSE,
    TRANSICAO,
    RESPAWNING,
    GAME_OVER,
    VITORIA,
    TELA_RANKING
};

EstadoJogo estadoAtual = MENU_PRINCIPAL;
EstadoJogo estadoAnterior = MENU_PRINCIPAL;

// Tipos de Itens
enum TipoItem {
    ITEM_NENHUM = 0,
    ITEM_VIDA_EXTRA = 1,
    ITEM_VELOCIDADE = 2,
    ITEM_TIRO_EXTRA = 3,
    ITEM_TIRO_MULTIPLO = 4,
    ITEM_PONTOS = 5,
    ITEM_CONGELA = 6
};

// Estruturas de Dados
struct Nave {
    float x, y;
    int vidas;
    int pontuacao;
    int tipoNave; // 1, 2, 3

    bool temTiroExtraTemp;
    bool temTiroMultiploTemp;
    bool temVelocidade;

    float tempoVelocidade;
    float tempoTiroExtra;
    float tempoTiroMultiplo;

    float cooldownTiro;
};

struct Projetil {
    float x, y;
    bool ativo;
    bool inimigo;
    int dx;
};

struct Inimigo {
    float x, y;
    bool vivo;
    int vida;
    int tipo;
};

struct Barreira {
    float x, y;
    int vida;
};

struct Particula {
    float x, y;
    float vida;
    sf::Color cor;
};

struct ItemDrop {
    float x, y;
    bool ativo;
    int tipo;
};

struct RankingEntry {
    string nome;
    int pontos;
    string data;
    int tempo;
    string modo;
};

//  Variáveis Globais
vector<Projetil> projeteis;
vector<Inimigo> inimigos;
vector<Barreira> barreiras;
vector<Particula> particulas;
ItemDrop itemAtual;

Nave jogador1;
Nave jogador2;
string nomeJogador1 = "";
string nomeJogador2 = "";

sf::Font fonteGlobal;

float tempoTotalJogo = 0.0f;
float tempoTransicao = 0.0f;
float tempoRespawn = 0.0f;

sf::SoundBuffer bufferTiro;
sf::SoundBuffer bufferExplosao;
std::list<sf::Sound> sonsAtivos;
sf::Music musicaFundo;

sf::Texture texNave1, texNave2, texNave3;
sf::Texture texIni1, texIni2, texIni3, texBoss;
sf::Texture texBarreira, texFundo, texLogo;
sf::Texture texItemVida, texItemVel, texItemTiroEx, texItemTiroMulti, texItemPontos, texItemCongela;

sf::Sprite sprNave1(texNave1);
sf::Sprite sprNave2(texNave2);
sf::Sprite sprNave3(texNave3);
sf::Sprite sprIni1(texIni1);
sf::Sprite sprIni2(texIni2);
sf::Sprite sprIni3(texIni3);
sf::Sprite sprBoss(texBoss);
sf::Sprite sprBarreira(texBarreira);
sf::Sprite sprFundo(texFundo);
sf::Sprite sprLogo(texLogo);

sf::Sprite sprItemVida(texItemVida);
sf::Sprite sprItemVel(texItemVel);
sf::Sprite sprItemTiroEx(texItemTiroEx);
sf::Sprite sprItemTiroMulti(texItemTiroMulti);
sf::Sprite sprItemPontos(texItemPontos);
sf::Sprite sprItemCongela(texItemCongela);

bool tNave1=false, tNave2=false, tNave3=false;
bool tIni1=false, tIni2=false, tIni3=false, tBoss=false;
bool tBar=false, tFundo=false, tLogo=false;
bool tIVida=false, tIVel=false, tITEx=false, tITMul=false, tIPts=false, tICong=false;

int faseAtual = 1;
int dificuldadeSelecionada = 2;
int menuOpcao = 0;

int abaRanking = 0;
const string NOMES_ABAS[] = {"CAMPANHA", "INFINITO", "DESAFIO", "DEMO"};

int paginaAjuda = 1;

bool modoMultiplayer = false;
bool modoNPC = false;
bool modoInfinito = false;
bool modoDesafio = false;

bool bossVivoGlobal = false;
int bossVidaGlobal = 0;
float dirInimigos = 1.0f;
float velocidadeInimigosBase = 0.1f;
float velocidadeAtual = 0.1f;

bool inimigosCongelados = false;
float tempoRestanteCongelamento = 0.0f;

float timerTiroInimigo = 0.0f;
float timerBoss = 0.0f;
float timerSpawnDesafio = 0.0f;

float desafioVelocidadeMultiplier = 1.0f;

// Funções Auxiliares

void configurarSprite(sf::Sprite &spr, sf::Texture &tex, float escalaX = 1.0f, float escalaY = 1.0f) {
    spr.setTexture(tex, true);
    sf::Vector2u tam = tex.getSize();
    if(tam.x == 0 || tam.y == 0) return;
    float scaleX = (TAM_CELULA * escalaX) / (float)tam.x;
    float scaleY = (TAM_CELULA * escalaY) / (float)tam.y;
    spr.setScale({scaleX, scaleY});
}

void carregarRecursos() {
    if (!fonteGlobal.openFromFile("arial.ttf")) {
        if (!fonteGlobal.openFromFile("C:/Windows/Fonts/arial.ttf")) cout << "AVISO: Fonte arial.ttf nao encontrada." << endl;
    }

    if(bufferTiro.loadFromFile("tiro.wav")) {} else cout << "AVISO: tiro.wav nao encontrado." << endl;
    if(bufferExplosao.loadFromFile("explosao.wav")) {} else cout << "AVISO: explosao.wav nao encontrado." << endl;

    if (!musicaFundo.openFromFile("musica.mp3")) {
    } else {
        musicaFundo.setLooping(true);
        musicaFundo.setVolume(20);
        musicaFundo.play();
    }

    auto loadT = [](sf::Texture& t, string s, sf::Sprite& sp, bool& f, float sx, float sy) {
        if(t.loadFromFile(s)) { f = true; configurarSprite(sp, t, sx, sy); }
    };

    loadT(texNave1, "nave1.png", sprNave1, tNave1, 2.0f, 2.0f);
    loadT(texNave2, "nave2.png", sprNave2, tNave2, 2.0f, 2.0f);
    loadT(texNave3, "nave3.png", sprNave3, tNave3, 2.0f, 2.0f);

    loadT(texIni1, "inimigo1.png", sprIni1, tIni1, 1.0f, 1.0f);
    loadT(texIni2, "inimigo2.png", sprIni2, tIni2, 1.0f, 1.0f);
    loadT(texIni3, "inimigo3.png", sprIni3, tIni3, 1.0f, 1.0f);
    loadT(texBoss, "boss.png", sprBoss, tBoss, 3.0f, 3.0f);

    loadT(texBarreira, "barreira.png", sprBarreira, tBar, 1.0f, 1.0f);

    if(texFundo.loadFromFile("fundo.png")) {
        tFundo = true; sprFundo.setTexture(texFundo, true);
        sf::Vector2u tam = texFundo.getSize();
        if(tam.x > 0 && tam.y > 0) sprFundo.setScale({(float)LARGURA_JANELA/tam.x, (float)ALTURA_JANELA/tam.y});
    }
    if (texLogo.loadFromFile("logo.png")) {
        tLogo = true; sprLogo.setTexture(texLogo, true);
        sf::Vector2u tam = texLogo.getSize();
        sprLogo.setOrigin({tam.x / 2.0f, tam.y / 2.0f});
        sprLogo.setPosition({LARGURA_JANELA / 2.0f, 120.0f});
    }

    loadT(texItemVida, "item_vida.png", sprItemVida, tIVida, 1.3f, 1.3f);
    loadT(texItemVel, "item_vel.png", sprItemVel, tIVel, 1.3f, 1.3f);
    loadT(texItemTiroEx, "item_tiro_extra.png", sprItemTiroEx, tITEx, 1.3f, 1.3f);
    loadT(texItemTiroMulti, "item_tiro_multi.png", sprItemTiroMulti, tITMul, 1.3f, 1.3f);
    loadT(texItemPontos, "item_pontos.png", sprItemPontos, tIPts, 1.3f, 1.3f);
    loadT(texItemCongela, "item_congela.png", sprItemCongela, tICong, 1.3f, 1.3f);
}

void tocarSom(const sf::SoundBuffer &buffer) {
    sonsAtivos.emplace_back(buffer);
    sonsAtivos.back().play();
}

void desenharSpriteOuQuad(sf::RenderWindow &window, float x, float y, sf::Color cor, bool temTex, sf::Sprite &spr, float escalaBase = 1.0f) {
    float telaX = x * TAM_CELULA;
    float telaY = y * TAM_CELULA + ALTURA_HUD;

    if(temTex) {
        spr.setPosition({telaX, telaY});
        if (cor == sf::Color::Yellow || cor == sf::Color(255, 140, 0)) {
             spr.setColor(cor);
        } else {
             spr.setColor(sf::Color::White);
        }
        window.draw(spr);
    } else {
        sf::RectangleShape rect(sf::Vector2f(TAM_CELULA * escalaBase - 2.0f, TAM_CELULA * escalaBase - 2.0f));
        rect.setPosition({telaX, telaY});
        rect.setFillColor(cor);
        window.draw(rect);
    }
}

void desenharTexto(sf::RenderWindow &window, string msg, float xPx, float yPx, unsigned int tamanho, sf::Color cor) {
    sf::Text text(fonteGlobal);
    text.setString(msg); text.setCharacterSize(tamanho); text.setFillColor(cor); text.setPosition({xPx, yPx});
    window.draw(text);
}

void desenharTextoCentralizado(sf::RenderWindow &window, string msg, float yPx, unsigned int tamanho, sf::Color cor) {
    sf::Text text(fonteGlobal);
    text.setString(msg); text.setCharacterSize(tamanho); text.setFillColor(cor);
    sf::FloatRect textRect = text.getLocalBounds();
    text.setOrigin({textRect.position.x + textRect.size.x / 2.0f, textRect.position.y + textRect.size.y / 2.0f});
    text.setPosition({LARGURA_JANELA / 2.0f, yPx});
    window.draw(text);
}

void desenharCaixa(sf::RenderWindow &window, float x, float y, float w, float h, sf::Color corFill, sf::Color corOut) {
    sf::RectangleShape rect({w, h});
    rect.setPosition({x, y});
    rect.setFillColor(corFill);
    rect.setOutlineColor(corOut);
    rect.setOutlineThickness(2.0f);
    window.draw(rect);
}

void salvarScore(int pontos) {
    ofstream arq("ranking.txt", ios::app);
    if(arq.is_open()){
        string nome;
        string modoStr = "CAMPANHA";
        if (modoNPC) {
            nome = "CPU"; modoStr = "DEMO";
        } else {
            if (nomeJogador1.empty()) nome = "Player 1"; else nome = nomeJogador1;
            if (modoMultiplayer) {
                nome += " & ";
                if (nomeJogador2.empty()) nome += "Player 2"; else nome += nomeJogador2;
            }
            if (modoDesafio) modoStr = "DESAFIO";
            else if (modoInfinito) modoStr = "INFINITO";
        }

        time_t t = time(nullptr); char buffer[20]; strftime(buffer, 20, "%d/%m/%Y", localtime(&t));
        int tempo = (int)tempoTotalJogo;
        arq << nome << ";" << pontos << ";" << buffer << ";" << tempo << ";" << modoStr << "\n";
        arq.close();
    }
}

vector<RankingEntry> lerRanking() {
    vector<RankingEntry> lista;
    ifstream arq("ranking.txt");
    string linha;
    while(getline(arq, linha)){
        stringstream ss(linha); string segment; vector<string> parts;
        while(getline(ss, segment, ';')) parts.push_back(segment);
        if(parts.size() >= 4){
            RankingEntry r; r.nome=parts[0];
            try{r.pontos=stoi(parts[1]);}catch(...){r.pontos=0;}
            r.data=parts[2];
            try{r.tempo=stoi(parts[3]);}catch(...){r.tempo=0;}
            if(parts.size() >= 5) r.modo = parts[4]; else r.modo = "CAMPANHA";
            lista.push_back(r);
        }
    }
    sort(lista.begin(), lista.end(), [](const RankingEntry& a, const RankingEntry& b) { return a.pontos > b.pontos; });
    return lista;
}

void criarExplosao(float x, float y, sf::Color cor = sf::Color::Yellow) {
    for(int i=0; i<5; i++){
        Particula p; p.x = x+(rand()%10-5)/10.0f; p.y = y+(rand()%10-5)/10.0f; p.vida = 1.0f; p.cor = sf::Color::Yellow;
        particulas.push_back(p);
    }
    tocarSom(bufferExplosao);
}

void prepararBarreiras() {
    barreiras.clear();
    if(modoDesafio) return;
    for(int k=0; k<3; k++) {
        float baseX = 5.0f + k * 12.0f; float baseY = ALTURA_GRID - 5.0f;
        for(int i=0; i<2; i++) {
            for(int j=0; j<3; j++) {
                Barreira b; b.x = baseX + j; b.y = baseY + i; b.vida = 2; barreiras.push_back(b);
            }
        }
    }
}

void inicializarFase(int fase) {
    inimigos.clear(); projeteis.clear(); particulas.clear(); barreiras.clear();
    itemAtual.ativo = false;

    bossVivoGlobal = false; bossVidaGlobal = 0;
    timerTiroInimigo = 0.0f;
    timerBoss = 0.0f;

    if (modoDesafio) {
        dirInimigos = 0; velocidadeAtual = 0.15f; desafioVelocidadeMultiplier = 1.0f;
        timerSpawnDesafio = 0.0f;
        return;
    }

    int vidaBase = (dificuldadeSelecionada == 1) ? 1 : (dificuldadeSelecionada == 3 ? 2 : 1);

    if (modoInfinito) {
        int colunas = 5; int linhas = 4;
        float startX = (LARGURA_GRID - (colunas * 4)) / 2.0f;
        for (int i = 0; i < linhas; i++) {
            for (int j = 0; j < colunas; j++) {
                Inimigo in; in.x = startX + j * 4.0f; in.y = 2.0f + i * 2.0f;
                in.vivo = true; in.vida = 1; in.tipo = i % 3; inimigos.push_back(in);
            }
        }
        dirInimigos = 1.0f; velocidadeInimigosBase = 0.1f; velocidadeAtual = velocidadeInimigosBase;
        prepararBarreiras();
        return;
    }

    int linhas = 3 + fase; if(linhas > 6) linhas = 6;
    int colunas = 3; if(fase == 2) colunas = 4; if(fase >= 3) colunas = 5;
    float startX = (LARGURA_GRID - (colunas * 4)) / 2.0f;

    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            Inimigo in; in.x = startX + j * 4.0f; in.y = 2.0f + i * 2.0f;
            in.vivo = true; in.vida = (i==0)? vidaBase+1 : vidaBase; in.tipo = i % 3; inimigos.push_back(in);
        }
    }

    dirInimigos = 1.0f;
    velocidadeInimigosBase = (dificuldadeSelecionada == 1 ? 0.05f : (dificuldadeSelecionada == 3 ? 0.15f : 0.1f));
    velocidadeAtual = velocidadeInimigosBase;

    if (fase == 3) {
        Inimigo boss; boss.x = LARGURA_GRID/2.0f; boss.y = 3.0f; boss.vivo = true;
        boss.vida = 5; boss.tipo = 99; inimigos.push_back(boss);
        bossVivoGlobal = true; bossVidaGlobal = 5;
    }
    prepararBarreiras();
}

void resetarNave(Nave &n, float xPos) {
    n.x = xPos; n.y = ALTURA_GRID - 2;
    n.vidas = (dificuldadeSelecionada == 1 ? 5 : (dificuldadeSelecionada == 2 ? 3 : 1));
    n.pontuacao = 0;
    n.temTiroExtraTemp = false; n.temTiroMultiploTemp = false; n.temVelocidade = false;
    n.tempoVelocidade = 0.0f; n.tempoTiroExtra = 0.0f; n.tempoTiroMultiplo = 0.0f;
    n.cooldownTiro = 0.0f;
}

void reposicionarNaves() {
    if (modoMultiplayer) {
        jogador1.x = LARGURA_GRID / 3.0f; jogador1.y = ALTURA_GRID - 2;
        jogador2.x = 2.0f * LARGURA_GRID / 3.0f; jogador2.y = ALTURA_GRID - 2;
    } else {
        jogador1.x = LARGURA_GRID / 2.0f; jogador1.y = ALTURA_GRID - 2;
    }
    projeteis.clear();
}

void resetarJogo() {
    faseAtual = 1;
    tempoTotalJogo = 0.0f;
    if (modoMultiplayer) {
        resetarNave(jogador1, LARGURA_GRID / 3.0f); resetarNave(jogador2, 2.0f * LARGURA_GRID / 3.0f);
        jogador2.tipoNave = jogador1.tipoNave;
    } else {
        resetarNave(jogador1, LARGURA_GRID / 2.0f); jogador2.vidas = 0;
    }
    inicializarFase(1);
}

void spawnItem(float x, float y) {
    if (modoInfinito || modoDesafio) return;
    if (!itemAtual.ativo && (rand() % 100 < 75)) {
        itemAtual.ativo = true; itemAtual.x = x; itemAtual.y = y; itemAtual.tipo = 1 + (rand() % 6);
    }
}

// -IA NPC
void controlarNPC(Nave &n) {
    float baseVel = 0.25f;
    if (n.tipoNave == 1) baseVel = 0.35f;
    if (n.tipoNave == 3) baseVel = 0.15f;

    float velocidadeNPC = n.temVelocidade ? baseVel * 1.5f : baseVel;

    float candidatos[3] = { -velocidadeNPC, 0.0f, velocidadeNPC };
    float scores[3] = { 0.0f, 0.0f, 0.0f };

    auto avaliarPerigo = [&](float posX) -> float {
        float perigoTotal = 0.0f;
        for (auto &p : projeteis) {
            if (p.ativo && p.inimigo) {
                if (abs(p.x - posX) < 2.2f) {
                    if (p.y < n.y + 3.0f) {
                        float distY = n.y - p.y;
                        if (distY < 25.0f) {
                            float peso = (abs(distY) < 5.0f) ? 1000000.0f : (50000.0f / (abs(distY) + 0.1f));
                            perigoTotal += peso;
                        }
                    }
                }
            }
        }
        return perigoTotal;
    };

    float alvoX = -1.0f;
    float maiorY = -1.0f;

    for (auto &in : inimigos) {
        if (in.vivo) {
            if (in.y > maiorY) {
                maiorY = in.y;
                alvoX = in.x;
            } else if (in.y == maiorY) {
                if (abs(in.x - n.x) < abs(alvoX - n.x)) alvoX = in.x;
            }
        }
    }

    if (itemAtual.ativo && abs(itemAtual.y - n.y) < 20.0f) {
        if (avaliarPerigo(itemAtual.x) < 1000.0f) {
            alvoX = itemAtual.x;
        }
    }

    for (int i = 0; i < 3; i++) {
        float futuroX = n.x + candidatos[i];
        if (futuroX < 1.0f || futuroX > LARGURA_GRID - 2.0f) scores[i] -= 50000.0f;
        scores[i] -= avaliarPerigo(futuroX);
        if (alvoX != -1.0f) {
            float distAlvo = abs(futuroX - alvoX);
            scores[i] -= distAlvo * 10.0f;
        }
        scores[i] -= abs(futuroX - (LARGURA_GRID/2.0f)) * 0.5f;
    }

    int melhorIndex = 1;
    if (scores[0] > scores[1] && scores[0] > scores[2]) melhorIndex = 0;
    if (scores[2] > scores[0] && scores[2] > scores[1]) melhorIndex = 2;
    n.x += candidatos[melhorIndex];

    bool barreiraFrente = false;
    if (!modoDesafio) {
        for (auto &b : barreiras) {
            if (b.vida > 0 && abs(b.x - n.x) < 0.8f && b.y < n.y) barreiraFrente = true;
        }
    }

    if (alvoX != -1.0f && abs(n.x - alvoX) < 1.2f && !barreiraFrente) {
        float cooldown = 0.25f;
        if (n.tipoNave == 2) cooldown = 0.5f;
        if (n.tipoNave == 3) cooldown = 1.0f;
        if (n.temTiroExtraTemp || n.temTiroMultiploTemp) cooldown = 0.2f;

        if (n.cooldownTiro <= 0.0f) {
            tocarSom(bufferTiro);
            Projetil p; p.ativo=true; p.inimigo=false; p.x=n.x; p.y=n.y-1; p.dx=0; projeteis.push_back(p);
            if (n.tipoNave == 2 || n.temTiroExtraTemp) { Projetil p2=p; p2.x+=0.5f; p2.y-=0.5f; projeteis.push_back(p2); }
            if (n.tipoNave == 3 || n.temTiroMultiploTemp) { Projetil pe=p; pe.dx=-1; projeteis.push_back(pe); Projetil pd=p; pd.dx=1; projeteis.push_back(pd); }
            n.cooldownTiro = cooldown;
        }
    }
}

void atualizarLogica() {
    tempoTotalJogo += DT_FIXO;

    if(modoDesafio) {
        desafioVelocidadeMultiplier += 0.0002f;
        timerSpawnDesafio += DT_FIXO;
        if (timerSpawnDesafio > (1.5f / desafioVelocidadeMultiplier)) {
            Inimigo in;
            in.x = (float)(rand() % (LARGURA_GRID - 2) + 1);
            in.y = 0; in.vivo = true; in.vida = 1;
            in.tipo = rand() % 3;
            inimigos.push_back(in);
            timerSpawnDesafio = 0.0f;
        }
    }

    auto moverNave = [](Nave &n, bool esq, bool dir) {
        float vel = 0.2f;
        if(n.tipoNave == 1) vel = 0.3f;
        if(n.tipoNave == 3) vel = 0.15f;
        if(n.temVelocidade) vel *= 1.5f;

        if (esq && n.x > 0) n.x -= vel;
        if (dir && n.x < LARGURA_GRID - 1) n.x += vel;
    };

    if (jogador1.cooldownTiro > 0.0f) jogador1.cooldownTiro -= DT_FIXO;
    if (modoMultiplayer && jogador2.cooldownTiro > 0.0f) jogador2.cooldownTiro -= DT_FIXO;

    if (modoNPC) {
        controlarNPC(jogador1);
    }
    else {
        bool l1=sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A), r1=sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D);
        if(!modoMultiplayer) {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) l1=true;
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) r1=true;
        }
        if(jogador1.vidas > 0) moverNave(jogador1, l1, r1);
        if(modoMultiplayer && jogador2.vidas > 0) moverNave(jogador2, sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left), sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right));
    }

    auto atirar = [](Nave &n) {
        float cooldown = 0.25f;
        if (n.tipoNave == 2) cooldown = 0.5f;
        if (n.tipoNave == 3) cooldown = 1.0f;
        if (n.temTiroExtraTemp || n.temTiroMultiploTemp) cooldown = 0.2f;

        if (n.cooldownTiro <= 0.0f) {
            tocarSom(bufferTiro);
            Projetil p; p.ativo=true; p.inimigo=false; p.x=n.x; p.y=n.y-1; p.dx=0; projeteis.push_back(p);
            if (n.tipoNave == 2 || n.temTiroExtraTemp) { Projetil p2=p; p2.x+=0.5f; p2.y-=0.5f; projeteis.push_back(p2); }
            if (n.tipoNave == 3 || n.temTiroMultiploTemp) { Projetil pe=p; pe.dx=-1; projeteis.push_back(pe); Projetil pd=p; pd.dx=1; projeteis.push_back(pd); }
            n.cooldownTiro = cooldown;
        }
    };

    if (!modoNPC && jogador1.vidas > 0 && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
        atirar(jogador1);
    }
    if (modoMultiplayer && jogador2.vidas > 0 && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
        atirar(jogador2);
    }

    if(inimigosCongelados) {
        tempoRestanteCongelamento -= DT_FIXO;
        if(tempoRestanteCongelamento <= 0.0f) inimigosCongelados = false;
    }

    if(!inimigosCongelados) {
        bool bateram = false; float minX = 1000, maxX = -1000; int vivos = 0;
        for(auto &in : inimigos) {
            if(!in.vivo) continue; vivos++;

            if(in.tipo == 99) {
                static float bossDir = 1.0f; in.x += bossDir * 0.15f;
                if(in.x > LARGURA_GRID-3 || in.x < 1) bossDir *= -1;

                timerBoss += DT_FIXO;
                if(timerBoss > 3.0f) {
                    for(int k=-1; k<=1; k++) { Projetil p; p.ativo=true; p.inimigo=true; p.x=in.x+k*2; p.y=in.y+2; p.dx=0; projeteis.push_back(p); }
                    timerBoss = 0.0f;
                }
                continue;
            }

            if(modoDesafio) {
                in.y += (velocidadeAtual * desafioVelocidadeMultiplier);
            } else {
                in.x += dirInimigos * velocidadeAtual;
                if(in.x < minX) minX = in.x;
                if(in.x > maxX) maxX = in.x;
            }

            if(in.y >= ALTURA_GRID-2) {
                if (modoDesafio) {
                    in.vivo = false; jogador1.vidas--;
                    if (jogador1.vidas <= 0) { estadoAtual = GAME_OVER; salvarScore(jogador1.pontuacao); }
                } else {
                    estadoAtual = GAME_OVER; salvarScore(jogador1.pontuacao + jogador2.pontuacao);
                }
            }
        }

        if(!modoDesafio && vivos > 0) velocidadeAtual = velocidadeInimigosBase + (1.0f / (float)vivos) * 0.2f;

        if(!modoDesafio) {
            if(vivos == 0) {
                if (modoInfinito) {
                    faseAtual++;
                    if(modoMultiplayer) {
                        int vidaBase = (dificuldadeSelecionada == 1 ? 5 : (dificuldadeSelecionada == 2 ? 3 : 1));
                        if(jogador1.vidas <= 0) jogador1.vidas = vidaBase;
                        if(jogador2.vidas <= 0) jogador2.vidas = vidaBase;
                    }
                    inicializarFase(faseAtual);
                }else {
                        if(faseAtual < 3) {
                            faseAtual++;

                            // Não alteramos jogador1.vidas nem jogador2.vidas.
                            // Eles vão para a próxima fase exatamente como terminaram esta.

                            // Reseta APENAS os itens/poderes temporários
                                jogador1.temTiroExtraTemp = false;
                                jogador1.temTiroMultiploTemp = false;
                                jogador1.temVelocidade = false;
                                jogador1.tempoVelocidade = 0.0f;
                                jogador1.tempoTiroExtra = 0.0f;
                                jogador1.tempoTiroMultiplo = 0.0f;

                                if(modoMultiplayer) {
                                    jogador2.temTiroExtraTemp = false;
                                    jogador2.temTiroMultiploTemp = false;
                                    jogador2.temVelocidade = false;
                                    jogador2.tempoVelocidade = 0.0f;
                                    jogador2.tempoTiroExtra = 0.0f;
                                    jogador2.tempoTiroMultiplo = 0.0f;
                                }

                                // Reposiciona as naves no centro/lados para começar a nova fase
                                reposicionarNaves();

                                estadoAtual = TRANSICAO;
                                tempoTransicao = 3.0f;
                            }
                            else {
                                estadoAtual = VITORIA;
                                salvarScore(jogador1.pontuacao + jogador2.pontuacao);
                            }
                        }
            }
            if(maxX >= LARGURA_GRID-1 || minX <= 0) bateram = true;
            if(bateram) { dirInimigos *= -1; for(auto &in : inimigos) if(in.tipo != 99) in.y += 1.0f; }
        }

        timerTiroInimigo += DT_FIXO;
        if(!modoDesafio && timerTiroInimigo > (2.0f/dificuldadeSelecionada) && vivos > 0) {
            int idx = rand() % inimigos.size();
            if(inimigos[idx].vivo) {
                Projetil p; p.ativo=true; p.inimigo=true; p.x=inimigos[idx].x; p.y=inimigos[idx].y+1; p.dx=0; projeteis.push_back(p);
                timerTiroInimigo = 0.0f;
            }
        }
    }

    for(auto &p : projeteis) {
        if(!p.ativo) continue;
        if(p.inimigo) p.y += 0.2f; else { p.y -= 0.5f; p.x += p.dx * 0.2f; }
        if(p.y < 0 || p.y > ALTURA_GRID) p.ativo = false;

        for(auto &b : barreiras) if(b.vida > 0 && p.ativo && abs(p.x - b.x) < 1.0f && abs(p.y - b.y) < 1.0f) { p.ativo = false; b.vida--; }

        auto hitPlayer = [&](Nave &n) {
            if(n.vidas > 0 && p.inimigo && p.ativo && abs(p.x - n.x) < 1.0f && abs(p.y - n.y) < 1.0f) {
                if(!n.temVelocidade) {
                    n.vidas--;
                    p.ativo = false;
                    criarExplosao(n.x, n.y, sf::Color::Green);

                    if (n.vidas > 0) {
                        estadoAtual = RESPAWNING;
                        tempoRespawn = 2.0f;
                    }
                }
            }
        };
        hitPlayer(jogador1); if(modoMultiplayer) hitPlayer(jogador2);

        if(jogador1.vidas <= 0 && (!modoMultiplayer || jogador2.vidas <= 0)) { estadoAtual = GAME_OVER; salvarScore(jogador1.pontuacao + jogador2.pontuacao); }

        if(!p.inimigo && p.ativo) {
            for(auto &in : inimigos) {

                // -DEFINIÇÃO DA HITBOX
                // Se for o Boss (tipo 99), a hitbox é maior (2.5 na largura, 2.0 na altura)
                // Se for inimigo comum, mantém o padrão 1.0
                float hitboxX = (in.tipo == 99) ? 2.5f : 1.0f;
                float hitboxY = (in.tipo == 99) ? 2.0f : 1.0f;

                // Verifica colisão usando as variáveis de tamanho dinâmico
                if(in.vivo && abs(p.x - in.x) < hitboxX && abs(p.y - in.y) < hitboxY) {

                    p.ativo = false;
                    in.vida--;
                    criarExplosao(in.x, in.y, sf::Color::Red);

                    if(in.vida <= 0) {
                        in.vivo = false;
                        int pts = (in.tipo == 99) ? 1000 : ((in.tipo+1) * 100);
                        jogador1.pontuacao += pts;

                        if(in.tipo == 99) { bossVivoGlobal = false; }
                        else if(in.tipo == 99 && bossVivoGlobal) { bossVidaGlobal = in.vida; }

                        spawnItem(in.x, in.y);
                    } else if(in.tipo == 99) {
                        bossVidaGlobal = in.vida;
                    }
                    break;
                }
            }
        }
    }

    if(itemAtual.ativo) {
        itemAtual.y += 0.15f; if(itemAtual.y > ALTURA_GRID) itemAtual.ativo = false;
        auto pegarItem = [&](Nave &n) {
            if(n.vidas > 0 && abs(itemAtual.x - n.x) < 1.5f && abs(itemAtual.y - n.y) < 1.0f) {
                itemAtual.ativo = false;
                switch(itemAtual.tipo) {
                    case ITEM_VIDA_EXTRA: n.vidas++; break;
                    case ITEM_VELOCIDADE: n.temVelocidade=true; n.tempoVelocidade = 8.0f; break;
                    case ITEM_TIRO_EXTRA: n.temTiroExtraTemp=true; n.tempoTiroExtra = 6.0f; break;
                    case ITEM_TIRO_MULTIPLO: n.temTiroMultiploTemp=true; n.tempoTiroMultiplo = 5.0f; break;
                    case ITEM_PONTOS: n.pontuacao += 500; break;
                    case ITEM_CONGELA: inimigosCongelados=true; tempoRestanteCongelamento = 2.0f; break;
                }
            }
        };
        pegarItem(jogador1); if(modoMultiplayer) pegarItem(jogador2);
    }

    auto checarTimer = [&](Nave &n) {
        if(n.temVelocidade) { n.tempoVelocidade -= DT_FIXO; if(n.tempoVelocidade <= 0) n.temVelocidade = false; }
        if(n.temTiroExtraTemp) { n.tempoTiroExtra -= DT_FIXO; if(n.tempoTiroExtra <= 0) n.temTiroExtraTemp = false; }
        if(n.temTiroMultiploTemp) { n.tempoTiroMultiplo -= DT_FIXO; if(n.tempoTiroMultiplo <= 0) n.temTiroMultiploTemp = false; }
    };
    checarTimer(jogador1); if(modoMultiplayer) checarTimer(jogador2);

    for(auto &pt : particulas) pt.vida -= 0.05f;
    particulas.erase(remove_if(particulas.begin(), particulas.end(), [](const Particula &p){ return p.vida <= 0; }), particulas.end());
}

// Desenho

void desenharMenuPrincipal(sf::RenderWindow &window) {
    if(tFundo) window.draw(sprFundo); else window.clear(sf::Color::Black);

    if(tLogo) {
        window.draw(sprLogo);
    } else {
        desenharTextoCentralizado(window, "SPACE INVADERS", 100.0f, 50, sf::Color::Cyan);
    }

    desenharCaixa(window, LARGURA_JANELA/2.0f - 200.0f, 200.0f, 400.0f, 350.0f, sf::Color(0,0,0,150), sf::Color::Cyan);

    string ops[] = {"CAMPANHA", "INFINITO", "DESAFIO (CHUVA)", "DEMO (NPC)", "RANKING", "AJUDA", "SAIR"};
    for(int i=0; i<7; i++) {
        sf::Color c = (i==menuOpcao ? sf::Color::Yellow : sf::Color::White);
        string t = (i==menuOpcao ? "> " + ops[i] + " <" : ops[i]);
        desenharTextoCentralizado(window, t, 240.0f + i*45, 20, c);
    }
    window.display();
}

void desenharAjuda(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    string titulo = "AJUDA (" + to_string(paginaAjuda) + "/3)";
    desenharTextoCentralizado(window, titulo, 20.0f, 30, sf::Color::Yellow);
    desenharCaixa(window, 50.0f, 60.0f, LARGURA_JANELA-100.0f, ALTURA_JANELA-120.0f, sf::Color(20,20,20), sf::Color::White);

    sf::Text text(fonteGlobal);
    text.setCharacterSize(16);
    text.setFillColor(sf::Color::White);
    text.setPosition({70.0f, 80.0f});

    stringstream ss;

    if (paginaAjuda == 1) {
        ss << "--- MODOS DE JOGO ---\n";
        ss << "CAMPANHA: 3 FASES + BOSS FINAL. DERROTE TODOS PARA VENCER.\n";
        ss << "  - FACIL/MEDIO: INIMIGOS DA ULTIMA FILEIRA = 2 VIDAS, OUTROS = 1 VIDA.\n";
        ss << "  - DIFICIL: INIMIGOS DA ULTIMA FILEIRA = 3 VIDAS, OUTROS = 2 VIDAS.\n";
        ss << "INFINITO: ONDAS SEM FIM (1 HP). SEM ITENS/BOSS.\n";
        ss << "DESAFIO: CHUVA DE ALIENS. NAO DEIXEM TOCAR O CHAO!\n\n";
        ss << "--- CONTROLES ---\n";
        ss << "P1: A/D OU SETAS (SOLO) | TIRO: ESPACO\n";
        ss << "P2: SETAS | TIRO: CIMA (APENAS MULTIPLAYER)\n";
    }
    else if (paginaAjuda == 2) {
        ss << "--- ITENS (CAMPANHA) ---\n";
        ss << "VIDA EXTRA\n";
        ss << "VELOCIDADE (8s)\n";
        ss << "TIRO EXTRA (6s)\n";
        ss << "TIRO TRIPLO(5s)\n";
        ss << "PONTOS\n";
        ss << "CONGELA INIMIGOS (2s)\n\n";
        ss << "--- NAVES E COOLDOWN ---\n";
        ss << "1. INTERCEPTOR (RAPIDA): 0.25s\n";
        ss << "2. DESTRUIDOR (TIRO DUPLO): 0.50s\n";
        ss << "3. CRUZADOR (TIRO TRIPLO): 1.00s\n";
    }
    else {
        ss << "--- CREDITOS ---\n\n";
        ss << "AUTORES:\n";
        ss << "- JOAO MATHEUS CACHOEIRA PIMENTAL\n";
        ss << "- LUCIANO TOMASI JUNIOR\n";
        ss << "- LUIZA ALMEIDA DEON\n\n";
        ss << "DISCIPLINA:\n";
        ss << "ALGORITIMOS E PROGRAMACAO II\n";
        ss << "UNIVALI\n";
    }

    text.setString(ss.str());
    window.draw(text);

    if (paginaAjuda > 1)
        desenharTexto(window, "<- SETA ESQUERDA", 60.0f, ALTURA_JANELA - 40.0f, 14, sf::Color::Cyan);

    if (paginaAjuda < 3)
        desenharTexto(window, "SETA DIREITA ->", LARGURA_JANELA - 180.0f, ALTURA_JANELA - 40.0f, 14, sf::Color::Cyan);

    desenharTextoCentralizado(window, "ESC ou ENTER para voltar", ALTURA_JANELA - 40.0f, 18, sf::Color::White);
    window.display();
}

void desenharMenuJogadores(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    desenharTextoCentralizado(window, "QUANTOS JOGADORES?", 150.0f, 40, sf::Color::White);
    string ops[] = {"1 JOGADOR", "2 JOGADORES"};
    for(int i=0; i<2; i++) desenharTextoCentralizado(window, (i==menuOpcao ? "> " : "") + ops[i] + (i==menuOpcao ? " <" : ""), 300.0f + i*50, 30, i==menuOpcao ? sf::Color::Green : sf::Color::White);
    desenharTextoCentralizado(window, "[ESC] Voltar", ALTURA_JANELA - 50.0f, 18, sf::Color::Cyan);
    window.display();
}

void desenharMenuModoNPC(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    desenharTextoCentralizado(window, "MODO DO NPC", 150.0f, 40, sf::Color::Yellow);
    string ops[] = {"CAMPANHA", "INFINITO", "DESAFIO"};
    for(int i=0; i<3; i++) desenharTextoCentralizado(window, (i==menuOpcao ? "> " : "") + ops[i] + (i==menuOpcao ? " <" : ""), 250.0f + i*50, 30, i==menuOpcao ? sf::Color::Green : sf::Color::White);
    desenharTextoCentralizado(window, "[ESC] Voltar", ALTURA_JANELA - 50.0f, 18, sf::Color::Cyan);
    window.display();
}

void desenharMenuDificuldade(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    desenharTextoCentralizado(window, "DIFICULDADE", 150.0f, 40, sf::Color::White);
    string ops[] = {"FACIL (5 Vidas)", "MEDIO (3 Vidas)", "DIFICIL (1 Vida)"};
    for(int i=0; i<3; i++) desenharTextoCentralizado(window, (i==menuOpcao ? "> " : "") + ops[i] + (i==menuOpcao ? " <" : ""), 250.0f + i*50, 30, i==menuOpcao ? sf::Color::Cyan : sf::Color::White);
    desenharTextoCentralizado(window, "[ESC] Voltar", ALTURA_JANELA - 50.0f, 18, sf::Color::Cyan);
    window.display();
}

void desenharSelecaoNave(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    string titulo = modoNPC ? "NAVE DO NPC" : "NAVE JOGADOR 1";
    desenharTextoCentralizado(window, titulo, 50.0f, 30, sf::Color::White);
    string ops[] = {"INTERCEPTOR (Rapida)", "DESTRUIDOR (Tiro Duplo)", "CRUZADOR (Tiro Multi)"};
    for(int i=0; i<3; i++) {
        float yText = 150.0f + i*150;
        desenharTextoCentralizado(window, (i==menuOpcao ? "> " : "") + ops[i], yText, 20, i==menuOpcao ? sf::Color::Yellow : sf::Color::White);
        sf::Sprite* s = &sprNave1;

        if(i==1) s = &sprNave2;
        if(i==2) s = &sprNave3;

        bool tem = (i==0?tNave1 : (i==1?tNave2 : tNave3));

        float xCentralizado = (LARGURA_GRID / 2.0f) - 1.0f;
        desenharSpriteOuQuad(window, xCentralizado, (yText + 40.0f)/TAM_CELULA - (ALTURA_HUD/TAM_CELULA), sf::Color::Green, tem, *s, 2.0f);
    }
    desenharTextoCentralizado(window, "[ESC] Voltar", ALTURA_JANELA - 50.0f, 18, sf::Color::Cyan);
    window.display();
}

void desenharTelaNome(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    string titulo = (estadoAtual == DIGITAR_NOME_P1) ? "DIGITE NOME JOGADOR 1" : "DIGITE NOME JOGADOR 2";
    desenharTextoCentralizado(window, titulo, 200.0f, 40, sf::Color::Cyan);
    string nomeAtual = (estadoAtual == DIGITAR_NOME_P1) ? nomeJogador1 : nomeJogador2;
    if ((clock() / 500) % 2 == 0) nomeAtual += "_";
    desenharTextoCentralizado(window, nomeAtual, 300.0f, 40, sf::Color::Yellow);
    desenharTextoCentralizado(window, "ENTER para confirmar", 500.0f, 20, sf::Color::White);
    desenharTextoCentralizado(window, "[ESC] Voltar", 550.0f, 20, sf::Color::Red);
    window.display();
}

void desenharRanking(sf::RenderWindow &window) {
    window.clear(sf::Color::Black);
    desenharTextoCentralizado(window, "RANKING - " + NOMES_ABAS[abaRanking], 30.0f, 40, sf::Color::Yellow);
    desenharTextoCentralizado(window, "< Use Setas Esquerda/Direita para mudar Modo >", 70.0f, 16, sf::Color::White);

    desenharTexto(window, "NOME", 50.0f, 110.0f, 20, sf::Color::Cyan);
    desenharTexto(window, "PONTOS", 460.0f, 110.0f, 20, sf::Color::Cyan);
    desenharTexto(window, "DATA", 660.0f, 110.0f, 20, sf::Color::Cyan);
    desenharTexto(window, "TEMPO", 860.0f, 110.0f, 20, sf::Color::Cyan);

    vector<RankingEntry> rk = lerRanking();
    float y = 150.0f; int c = 0;
    for(auto &r : rk) {
        if (r.modo == NOMES_ABAS[abaRanking]) {
            if(c++ >= 10) break;
            desenharTexto(window, r.nome, 50.0f, y, 18, sf::Color::White);
            desenharTexto(window, to_string(r.pontos), 460.0f, y, 18, sf::Color::White);
            desenharTexto(window, r.data, 660.0f, y, 18, sf::Color::White);
            desenharTexto(window, to_string(r.tempo)+"s", 860.0f, y, 18, sf::Color::White);
            y += 30.0f;
        }
    }
    desenharTextoCentralizado(window, "ESC para voltar", ALTURA_JANELA - 50.0f, 20, sf::Color::Cyan);
    window.display();
}

// JOGO DESENHO - Definido ANTES de Respawn e Pause
void desenharJogo(sf::RenderWindow &window) {
    if(tFundo) window.draw(sprFundo); else window.clear(sf::Color::Black);

    sf::RectangleShape hudBg({(float)LARGURA_JANELA, ALTURA_HUD});
    hudBg.setFillColor(sf::Color(0,0,0,200));
    hudBg.setOutlineThickness(2.0f);
    hudBg.setOutlineColor(sf::Color::Blue);
    window.draw(hudBg);

    sf::RectangleShape borda({(float)LARGURA_JANELA - 20.0f, (float)ALTURA_JANELA - ALTURA_HUD - 20.0f});
    borda.setPosition({10.0f, ALTURA_HUD + 10.0f});
    borda.setFillColor(sf::Color::Transparent);
    borda.setOutlineColor(sf::Color::White);
    borda.setOutlineThickness(2.0f);
    window.draw(borda);

    // SCORE E TEMPO
    stringstream ssGlobal;
    ssGlobal << "SCORE: " << (jogador1.pontuacao + jogador2.pontuacao)
             << "   TEMPO: " << (int)tempoTotalJogo << "s";
    desenharTextoCentralizado(window, ssGlobal.str(), 15.0f, 18, sf::Color::White);

    // Boss HP
    if(bossVivoGlobal) {
        stringstream bossSS;
        bossSS << "BOSS HP: " << bossVidaGlobal;
        desenharTextoCentralizado(window, bossSS.str(), 40.0f, 18, sf::Color::Red);
    }

    // STATUS GLOBAL
    if(inimigosCongelados) {
        int r = (int)tempoRestanteCongelamento;
        if (r < 0) r = 0;
        desenharTextoCentralizado(window, "CONGELADO: " + to_string(r) + "s", 65.0f, 16, sf::Color::Cyan);
    }

    // JOGADOR 1
    stringstream ssVidas1;
    ssVidas1 << "J1 VIDAS: " << jogador1.vidas;
    desenharTexto(window, ssVidas1.str(), 20.0f, 10.0f, 16, sf::Color::Green);

    vector<string> statusP1;
    auto getRestante = [](float t) -> string {
        int r = (int)t; if(r<0) r=0; return to_string(r) + "s";
    };

    if(jogador1.temVelocidade) statusP1.push_back("VEL: " + getRestante(jogador1.tempoVelocidade));
    if(jogador1.temTiroExtraTemp) statusP1.push_back("T.EXTRA: " + getRestante(jogador1.tempoTiroExtra));
    if(jogador1.temTiroMultiploTemp) statusP1.push_back("T.MULTI: " + getRestante(jogador1.tempoTiroMultiplo));

    float stY1 = 35.0f;
    for(auto &s : statusP1) {
        desenharTexto(window, s, 20.0f, stY1, 12, sf::Color::Green);
        stY1 += 25.0f;
    }

    // JOGADOR 2
    if(modoMultiplayer) {
        stringstream ssVidas2;
        ssVidas2 << "J2 VIDAS: " << jogador2.vidas;
        float xP2 = (float)LARGURA_JANELA - 150.0f;
        sf::Color corP2 = sf::Color(255, 140, 0);
        desenharTexto(window, ssVidas2.str(), xP2, 10.0f, 16, corP2);

        vector<string> statusP2;
        if(jogador2.temVelocidade) statusP2.push_back("VEL: " + getRestante(jogador2.tempoVelocidade));
        if(jogador2.temTiroExtraTemp) statusP2.push_back("T.EXTRA: " + getRestante(jogador2.tempoTiroExtra));
        if(jogador2.temTiroMultiploTemp) statusP2.push_back("T.MULTI: " + getRestante(jogador2.tempoTiroMultiplo));

        float stY2 = 35.0f;
        for(auto &s : statusP2) {
            desenharTexto(window, s, xP2, stY2, 12, corP2);
            stY2 += 25.0f;
        }
    }

    // ENTIDADES
    for(auto &in : inimigos) if(in.vivo) {
        sf::Sprite* spr = &sprIni1; bool tem = tIni1;
        if(in.tipo == 1) { spr = &sprIni2; tem = tIni2; }
        if(in.tipo == 2) { spr = &sprIni3; tem = tIni3; }
        if(in.tipo == 99) { spr = &sprBoss; tem = tBoss; }
        desenharSpriteOuQuad(window, in.x, in.y, in.tipo==99?sf::Color::Magenta:sf::Color::Red, tem, *spr);
    }

    for(auto &b : barreiras) if(b.vida > 0) desenharSpriteOuQuad(window, b.x, b.y, sf::Color(100, 100, 100), tBar, sprBarreira);

    auto desenharPlayer = [&](Nave &n, sf::Color c) {
        sf::Sprite* s = &sprNave1; bool t = tNave1;
        if(n.tipoNave==2) { s=&sprNave2; t=tNave2; }
        if(n.tipoNave==3) { s=&sprNave3; t=tNave3; }
        desenharSpriteOuQuad(window, n.x - 0.5f, n.y, c, t, *s, 2.0f);
    };

    if(jogador1.vidas > 0) desenharPlayer(jogador1, modoNPC?sf::Color::Yellow:sf::Color::White);
    if(modoMultiplayer && jogador2.vidas > 0) desenharPlayer(jogador2, sf::Color(255, 140, 0));

    for(auto &p : projeteis) if(p.ativo) {
        sf::RectangleShape s(sf::Vector2f(4, 10));
        s.setPosition({p.x*TAM_CELULA+8, p.y*TAM_CELULA + ALTURA_HUD});
        s.setFillColor(p.inimigo ? sf::Color::Red : sf::Color::Green);
        window.draw(s);
    }

    for(auto &pt : particulas) {
        sf::RectangleShape s(sf::Vector2f(4, 4));
        s.setPosition({pt.x*TAM_CELULA, pt.y*TAM_CELULA + ALTURA_HUD});
        sf::Color c = pt.cor; c.a = (std::uint8_t)(pt.vida * 255); s.setFillColor(c);
        window.draw(s);
    }

    if(itemAtual.ativo) {
        sf::Sprite* s = &sprItemPontos; bool t = tIPts;
        if(itemAtual.tipo==ITEM_VIDA_EXTRA) { s=&sprItemVida; t=tIVida; }
        if(itemAtual.tipo==ITEM_VELOCIDADE) { s=&sprItemVel; t=tIVel; }
        if(itemAtual.tipo==ITEM_TIRO_EXTRA) { s=&sprItemTiroEx; t=tITEx; }
        if(itemAtual.tipo==ITEM_TIRO_MULTIPLO) { s=&sprItemTiroMulti; t=tITMul; }
        if(itemAtual.tipo==ITEM_CONGELA) { s=&sprItemCongela; t=tICong; }
        desenharSpriteOuQuad(window, itemAtual.x, itemAtual.y, sf::Color::White, t, *s, 1.3f);
    }
}

void desenharTransicao(sf::RenderWindow &window) {
    sf::RectangleShape overlay({(float)LARGURA_JANELA, (float)ALTURA_JANELA});
    overlay.setFillColor(sf::Color::Black);
    window.draw(overlay);

    string msg = "FASE " + to_string(faseAtual);
    desenharTextoCentralizado(window, msg, 250.0f, 60, sf::Color::Yellow);

    stringstream ss;
    ss << "PREPARE-SE: " << fixed << setprecision(1) << tempoTransicao << "s";
    desenharTextoCentralizado(window, ss.str(), 350.0f, 30, sf::Color::White);

    window.display();
}

void desenharRespawn(sf::RenderWindow &window) {
    desenharJogo(window);

    sf::RectangleShape overlay({(float)LARGURA_JANELA, (float)ALTURA_JANELA});
    overlay.setFillColor(sf::Color(255, 0, 0, 100));
    window.draw(overlay);

    desenharTextoCentralizado(window, "ATINGIDO!", 250.0f, 50, sf::Color::Yellow);
    desenharTextoCentralizado(window, "REINICIANDO...", 350.0f, 30, sf::Color::White);

    window.display();
}

void desenharMenuPause(sf::RenderWindow &window) {
    sf::RectangleShape overlay({(float)LARGURA_JANELA, (float)ALTURA_JANELA});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(overlay);

    desenharCaixa(window, LARGURA_JANELA/2.0f - 150.0f, 250.0f, 300.0f, 250.0f, sf::Color(20,20,20), sf::Color::White);
    desenharTextoCentralizado(window, "JOGO PAUSADO", 200.0f, 30, sf::Color::Yellow);

    string ops[] = {"CONTINUAR", "AJUDA", "MENU PRINCIPAL", "SAIR DO JOGO"};
    for(int i=0; i<4; i++) {
        sf::Color c = (i==menuOpcao ? sf::Color::Green : sf::Color::White);
        string t = (i==menuOpcao ? "> " + ops[i] + " <" : ops[i]);
        desenharTextoCentralizado(window, t, 300.0f + i*40, 20, c);
    }
    window.display();
}

void desenharFim(sf::RenderWindow &window, bool v) {
    if(tFundo) window.draw(sprFundo); else window.clear(sf::Color::Black);
    sf::RectangleShape overlay({(float)LARGURA_JANELA, (float)ALTURA_JANELA});
    overlay.setFillColor(sf::Color(0,0,0,200));
    window.draw(overlay);

    desenharTextoCentralizado(window, v ? "VITORIA!" : "GAME OVER", 200, 50, v ? sf::Color::Green : sf::Color::Red);
    stringstream ss; ss << "PONTUACAO FINAL: " << (jogador1.pontuacao + jogador2.pontuacao);
    desenharTextoCentralizado(window, ss.str(), 300, 30, sf::Color::White);

    desenharTextoCentralizado(window, "Pressione ENTER para voltar ao Menu", 500, 20, sf::Color::Cyan);
    window.display();
}

int main() {
    sf::RenderWindow window(sf::VideoMode({LARGURA_JANELA, ALTURA_JANELA}), "Space Invaders");
    window.setFramerateLimit(60);
    carregarRecursos();

    while(window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                if (estadoAtual == DIGITAR_NOME_P1 || estadoAtual == DIGITAR_NOME_P2) {
                    string* alvo = (estadoAtual == DIGITAR_NOME_P1) ? &nomeJogador1 : &nomeJogador2;
                    char c = static_cast<char>(textEvent->unicode);
                    if (c >= 32 && c < 128 && alvo->length() < 10) *alvo += c;
                }
            }

            if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>()) {
                sf::Keyboard::Key k = keyEvent->code;

                if (k == sf::Keyboard::Key::Escape) {
                    if (estadoAtual == JOGANDO) {
                        estadoAtual = PAUSE;
                        menuOpcao = 0;
                        continue;
                    }
                    if (estadoAtual == PAUSE) {
                        estadoAtual = JOGANDO;
                        continue;
                    }
                    if (estadoAtual == MENU_JOGADORES || estadoAtual == MENU_MODO_NPC || estadoAtual == TELA_RANKING) {
                        estadoAtual = MENU_PRINCIPAL;
                        menuOpcao = 0;
                        continue;
                    }
                    if (estadoAtual == TELA_AJUDA) {
                        estadoAtual = estadoAnterior;
                        menuOpcao = 0;
                        continue;
                    }
                    if (estadoAtual == MENU_DIFICULDADE) {
                         estadoAtual = modoNPC ? MENU_MODO_NPC : MENU_JOGADORES;
                         menuOpcao = 0;
                         continue;
                    }
                    if (estadoAtual == SELECAO_NAVE) {
                        estadoAtual = MENU_DIFICULDADE;
                        menuOpcao = 0;
                        continue;
                    }
                    if (estadoAtual == DIGITAR_NOME_P1) {
                         if (modoInfinito || modoDesafio) estadoAtual = MENU_DIFICULDADE;
                         else estadoAtual = SELECAO_NAVE;
                         menuOpcao = 0;
                         continue;
                    }
                     if (estadoAtual == DIGITAR_NOME_P2) {
                         estadoAtual = DIGITAR_NOME_P1;
                         menuOpcao = 0;
                         continue;
                    }
                }

                if (estadoAtual == DIGITAR_NOME_P1 || estadoAtual == DIGITAR_NOME_P2) {
                      string* alvo = (estadoAtual == DIGITAR_NOME_P1) ? &nomeJogador1 : &nomeJogador2;
                      if (k == sf::Keyboard::Key::Backspace && !alvo->empty()) alvo->pop_back();
                      if (k == sf::Keyboard::Key::Enter) {
                          if (estadoAtual == DIGITAR_NOME_P1) {
                              if (modoMultiplayer) { estadoAtual = DIGITAR_NOME_P2; nomeJogador2 = ""; }
                              else { resetarJogo(); estadoAtual = JOGANDO; }
                          } else {
                              resetarJogo(); estadoAtual = JOGANDO;
                          }
                      }
                      continue;
                }

                if (estadoAtual == TELA_AJUDA) {
                    if(k == sf::Keyboard::Key::Right) { if(paginaAjuda < 3) paginaAjuda++; }
                    if(k == sf::Keyboard::Key::Left) { if(paginaAjuda > 1) paginaAjuda--; }
                    if(k == sf::Keyboard::Key::Enter) estadoAtual = estadoAnterior;
                    continue;
                }

                if (estadoAtual == TELA_RANKING) {
                    if(k == sf::Keyboard::Key::Right) { abaRanking++; if(abaRanking > 3) abaRanking = 0; }
                    if(k == sf::Keyboard::Key::Left) { abaRanking--; if(abaRanking < 0) abaRanking = 3; }
                    if(k == sf::Keyboard::Key::Enter) estadoAtual = MENU_PRINCIPAL;
                }

                if(estadoAtual != JOGANDO && estadoAtual != TELA_RANKING && estadoAtual != TELA_AJUDA) {
                    if(k == sf::Keyboard::Key::Up) menuOpcao--;
                    if(k == sf::Keyboard::Key::Down) menuOpcao++;
                }

                if (estadoAtual == PAUSE) {
                    if (menuOpcao < 0) menuOpcao = 3; if (menuOpcao > 3) menuOpcao = 0;
                    if (k == sf::Keyboard::Key::Enter) {
                        if (menuOpcao == 0) estadoAtual = JOGANDO;
                        if (menuOpcao == 1) { estadoAnterior = PAUSE; estadoAtual = TELA_AJUDA; paginaAjuda = 1; }
                        if (menuOpcao == 2) { estadoAtual = MENU_PRINCIPAL; menuOpcao = 0; }
                        if (menuOpcao == 3) window.close();
                    }
                }

                else if(estadoAtual == MENU_PRINCIPAL) {
                    if(menuOpcao < 0) menuOpcao = 6; if(menuOpcao > 6) menuOpcao = 0;
                    if(k == sf::Keyboard::Key::Enter) {
                        if(menuOpcao == 0) { modoInfinito = false; modoNPC = false; modoDesafio = false; estadoAtual = MENU_JOGADORES; menuOpcao=0; }
                        if(menuOpcao == 1) { modoInfinito = true; modoNPC = false; modoDesafio = false; estadoAtual = MENU_JOGADORES; menuOpcao=0; }
                        if(menuOpcao == 2) { modoInfinito = false; modoNPC = false; modoDesafio = true; estadoAtual = MENU_JOGADORES; menuOpcao=0; }
                        if(menuOpcao == 3) { modoNPC = true; modoMultiplayer = false; modoDesafio = false; estadoAtual = MENU_MODO_NPC; menuOpcao=0; }
                        if(menuOpcao == 4) { estadoAtual = TELA_RANKING; }
                        if(menuOpcao == 5) { estadoAnterior = MENU_PRINCIPAL; estadoAtual = TELA_AJUDA; paginaAjuda = 1; }
                        if(menuOpcao == 6) window.close();
                    }
                }
                else if(estadoAtual == MENU_JOGADORES) {
                    if(menuOpcao < 0) menuOpcao = 1; if(menuOpcao > 1) menuOpcao = 0;
                    if(k == sf::Keyboard::Key::Enter) {
                        modoMultiplayer = (menuOpcao == 1);
                        estadoAtual = MENU_DIFICULDADE; menuOpcao=0;
                    }
                }
                else if(estadoAtual == MENU_MODO_NPC) {
                    if(menuOpcao < 0) menuOpcao = 2; if(menuOpcao > 2) menuOpcao = 0;
                    if(k == sf::Keyboard::Key::Enter) {
                        if(menuOpcao == 0) { modoInfinito = false; modoDesafio = false; }
                        if(menuOpcao == 1) { modoInfinito = true; modoDesafio = false; }
                        if(menuOpcao == 2) { modoInfinito = false; modoDesafio = true; }
                        estadoAtual = MENU_DIFICULDADE; menuOpcao=0;
                    }
                }
                else if(estadoAtual == MENU_DIFICULDADE) {
                    if(menuOpcao < 0) menuOpcao = 2; if(menuOpcao > 2) menuOpcao = 0;
                    if(k == sf::Keyboard::Key::Enter) {
                        dificuldadeSelecionada = menuOpcao + 1;
                        if(modoInfinito || modoDesafio) {
                            jogador1.tipoNave = 1;
                            if (modoNPC) { resetarJogo(); estadoAtual = JOGANDO; }
                            else { estadoAtual = DIGITAR_NOME_P1; nomeJogador1 = ""; }
                        } else {
                            estadoAtual = SELECAO_NAVE; menuOpcao=0;
                        }
                    }
                }
                else if(estadoAtual == SELECAO_NAVE) {
                    if(menuOpcao < 0) menuOpcao = 2; if(menuOpcao > 2) menuOpcao = 0;
                    if(k == sf::Keyboard::Key::Enter) {
                        jogador1.tipoNave = menuOpcao + 1;
                        if (modoNPC) { resetarJogo(); estadoAtual = JOGANDO; }
                        else { estadoAtual = DIGITAR_NOME_P1; nomeJogador1 = ""; }
                    }
                }
                else if(estadoAtual == GAME_OVER || estadoAtual == VITORIA) {
                    if(k == sf::Keyboard::Key::Enter) { estadoAtual = MENU_PRINCIPAL; menuOpcao = 0; }
                }
            }
        }

        sonsAtivos.remove_if([](const sf::Sound& s) { return s.getStatus() == sf::Sound::Status::Stopped; });

        if(estadoAtual == MENU_PRINCIPAL) desenharMenuPrincipal(window);
        else if(estadoAtual == TELA_AJUDA) desenharAjuda(window);
        else if(estadoAtual == MENU_JOGADORES) desenharMenuJogadores(window);
        else if(estadoAtual == MENU_MODO_NPC) desenharMenuModoNPC(window);
        else if(estadoAtual == MENU_DIFICULDADE) desenharMenuDificuldade(window);
        else if(estadoAtual == SELECAO_NAVE) desenharSelecaoNave(window);
        else if(estadoAtual == DIGITAR_NOME_P1 || estadoAtual == DIGITAR_NOME_P2) desenharTelaNome(window);
        else if(estadoAtual == TELA_RANKING) desenharRanking(window);
        else if(estadoAtual == JOGANDO) {
            atualizarLogica();
            desenharJogo(window);
            window.display();
        }
        else if(estadoAtual == PAUSE) {
            desenharJogo(window);
            desenharMenuPause(window);
        }
        else if(estadoAtual == TRANSICAO) {
            tempoTransicao -= DT_FIXO;
            if(tempoTransicao <= 0.0f) {
                inicializarFase(faseAtual);
                estadoAtual = JOGANDO;
            }
            desenharTransicao(window);
        }
        else if(estadoAtual == RESPAWNING) {
            tempoRespawn -= DT_FIXO;
            if(tempoRespawn <= 0.0f) {
                reposicionarNaves();
                estadoAtual = JOGANDO;
            }
            desenharRespawn(window);
        }
        else if(estadoAtual == GAME_OVER) desenharFim(window, false);
        else if(estadoAtual == VITORIA) desenharFim(window, true);
    }
    return 0;
}
