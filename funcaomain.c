#include <stdio.h>
#include <string.h>

#define TAM 10
#define ARQ_USUARIOS "usuarios.bin"
#define ARQ_VIDEOS "videos.bin"

struct VideoYoutube {
    char titulo[100];
    char linkVideo[200];
    int idVideo;
};

struct Usuario {
    char nome[100];
    char email[100];
    int idUsuario;
    int idVideosFavoritos[TAM];
    int qtdFavoritos;
};

/* Prototipos das funcoes principais */
int CadastrarUsuario(
    struct Usuario *U,
    int idUsuario,
    const char nome[],
    const char email[],
    const int favoritos[],
    int qtdFavoritos
);
int CadastrarVideo(
    struct VideoYoutube *YV,
    int idVideo,
    const char titulo[],
    const char linkVideo[]
);
int GravarVideoBinario(const struct VideoYoutube *YV, const char *nomeArquivo);
int GravarUsuarioBinario(const struct Usuario *U, const char *nomeArquivo);
void ListarVideos(const char *nomeArquivo);
void ListarUsuarios(const char *nomeArquivo);
int RemoverVideoBinario(const char *nomeArquivo, int idRemover);
int BuscarVideoPorId(const char *nomeArquivo, int idBuscado, struct VideoYoutube *resultado);
void ExibirVideosFavoritosUsuario(const struct Usuario *U, const char *nomeArquivoVideos);
int AtualizarVideoBinario(const char *nomeArquivo, struct VideoYoutube novoVideo);
int AtualizarUsuarioBinario(const char *nomeArquivo, struct Usuario novoUsuario);
void RelatorioUsuariosComFavoritos(const char *nomeArquivoUsuarios, const char *nomeArquivoVideos);

/* Prototipos das funcoes auxiliares internas do menu */
static void LimparBufferEntrada(void);
static void LerLinha(char *destino, size_t tamanho);
static void ExibirMenuPrincipal(void);
static int LerOpcaoMenu(int *opcao);
static int BuscarUsuarioPorId(const char *nomeArquivo, int idBuscado, struct Usuario *resultado);
static void OpcaoCadastrarVideo(void);
static void OpcaoAtualizarVideo(void);
static void OpcaoRemoverVideo(void);
static void OpcaoCadastrarUsuario(void);
static void OpcaoAtualizarUsuario(void);
static void OpcaoExibirFavoritos(void);
static void OpcaoAdicionarFavorito(void);
static void OpcaoRemoverFavorito(void);
static int AlterarFavoritoUsuario(int idUsuario, int idVideo, int adicionar);
static int VerificarFormatoVideos(FILE *arq, const char *nomeArquivo);

int CadastrarUsuario(
    struct Usuario *U,
    int idUsuario,
    const char nome[],
    const char email[],
    const int favoritos[],
    int qtdFavoritos
) {
    if (U == NULL || nome == NULL || email == NULL) {
        return 0;
    }

    if (qtdFavoritos > 0 && favoritos == NULL) {
        return 0;
    }

    int limite = qtdFavoritos;

    if (limite < 0)
        limite = 0;

    if (limite > TAM)
        limite = TAM;

    memset(U, 0, sizeof(*U));

    U->idUsuario = idUsuario;

    strncpy(U->nome, nome, sizeof(U->nome) - 1);
    U->nome[sizeof(U->nome) - 1] = '\0';

    strncpy(U->email, email, sizeof(U->email) - 1);
    U->email[sizeof(U->email) - 1] = '\0';

    for (int i = 0; i < limite; i++) {
        U->idVideosFavoritos[i] = favoritos[i];
    }

    U->qtdFavoritos = limite;

    return 1;
}

int CadastrarVideo(
    struct VideoYoutube *YV,
    int idVideo,
    const char titulo[],
    const char linkVideo[]
) {
    if (YV == NULL || titulo == NULL || linkVideo == NULL) {
        return 0;
    }

    memset(YV, 0, sizeof(*YV));

    YV->idVideo = idVideo;

    strncpy(YV->titulo, titulo, sizeof(YV->titulo) - 1);
    YV->titulo[sizeof(YV->titulo) - 1] = '\0';

    strncpy(YV->linkVideo, linkVideo, sizeof(YV->linkVideo) - 1);
    YV->linkVideo[sizeof(YV->linkVideo) - 1] = '\0';

    return 1;
}

/* Impede leitura ou escrita sobre arquivos com registros de tamanho incompatível.
   Preserva a posicao atual para que os chamadores possam continuar a operacao. */
static int VerificarFormatoVideos(FILE *arq, const char *nomeArquivo) {
    long posicao, tamanho;
    long registro = (long)sizeof(struct VideoYoutube);
    if (arq == NULL || registro <= 0) return 0;

    posicao = ftell(arq);
    if (posicao < 0 || fseek(arq, 0, SEEK_END) != 0) {
        printf("Erro ao verificar %s.\n", nomeArquivo);
        return 0;
    }
    tamanho = ftell(arq);
    if (tamanho < 0 || fseek(arq, posicao, SEEK_SET) != 0) {
        printf("Erro ao verificar %s.\n", nomeArquivo);
        return 0;
    }
    if (tamanho % registro != 0) {
        printf("ERRO: %s possui formato incompatível (%ld bytes; esperado multiplo de %ld).\n",
               nomeArquivo, tamanho, registro);
        printf("Preserve uma copia do arquivo antes de tentar recupera-lo.\n");
        return 0;
    }
    return 1;
}

int GravarVideoBinario(
    const struct VideoYoutube *YV,
    const char *nomeArquivo
) {
    if (YV == NULL || nomeArquivo == NULL) {
        return 0;
    }

     /* Se ja existe, recusa acrescentar registros a um arquivo incompatível. */
    FILE *existente = fopen(nomeArquivo, "rb");
    if (existente != NULL) {
        int formatoOk = VerificarFormatoVideos(existente, nomeArquivo);
        fclose(existente);
        if (!formatoOk) return 0;
    }

    FILE *arq = fopen(nomeArquivo, "ab");

    if (arq == NULL) {
        printf("Erro ao abrir %s para escrita.\n", nomeArquivo);
        return 0;
    }

    size_t escritos = fwrite(
        YV,
        sizeof(struct VideoYoutube),
        1,
        arq
    );

    int fechamento = fclose(arq);

    if (escritos != 1 || fechamento != 0) {
        printf("Erro ao gravar video.\n");
        return 0;
    }

    return 1;
}

int GravarUsuarioBinario(
    const struct Usuario *U,
    const char *nomeArquivo
) {
    if (U == NULL || nomeArquivo == NULL) {
        return 0;
    }

    FILE *arq = fopen(nomeArquivo, "ab");

    if (arq == NULL) {
        printf("Erro ao abrir %s para escrita.\n", nomeArquivo);
        return 0;
    }

    size_t escritos = fwrite(
        U,
        sizeof(struct Usuario),
        1,
        arq
    );

    int fechamento = fclose(arq);

    if (escritos != 1 || fechamento != 0) {
        printf("Erro ao gravar usuario.\n");
        return 0;
    }

    return 1;
}

void ListarVideos(const char *nomeArquivo) {
    FILE *arq = fopen(nomeArquivo, "rb");

    if (arq == NULL) {
        printf("Erro ao listar os videos,tente de novo!\n");
        return;
    }

    if (!VerificarFormatoVideos(arq, nomeArquivo)) {
        fclose(arq);
        return;
    }

    struct VideoYoutube video;
    int contador = 0;

    while (fread(&video, sizeof(struct VideoYoutube), 1, arq) == 1) {
        printf("\n------LISTA DE VIDEOS %d------\n", ++contador);
        printf("ID: %d\n", video.idVideo);
        printf("Titulo: %s\n", video.titulo);
        printf("Link do video: %s\n", video.linkVideo);
    }

    if (ferror(arq)) {
        printf("Erro durante a leitura do arquivo.\n");
    } else if (contador == 0) {
        printf("Nenhum video está cadastrado!\n");
    }

    fclose(arq);
}

void ListarUsuarios(const char *nomeArquivo) {
    FILE *arq = fopen(nomeArquivo, "rb");

    if (arq == NULL) {
        printf("Erro ao listar os usuarios,tente de novo!\n");
        return;
    }

    struct Usuario usuario;
    int contador = 0;

    while (fread(&usuario, sizeof(struct Usuario), 1, arq) == 1) {
        int limite = usuario.qtdFavoritos;
        if (limite < 0) limite = 0;
        if (limite > TAM) limite = TAM;

        printf("\n------ LISTA DE USUARIOS %d ------\n", ++contador);
        printf("ID: %d\n", usuario.idUsuario);
        printf("Nome: %s\n", usuario.nome);
        printf("Email: %s\n", usuario.email);
        printf("Quantidade de favoritos: %d\n", limite);
        if (limite > 0) {
            printf("Videos favoritos: ");
            for (int i = 0; i < limite; i++) {
                printf("%d", usuario.idVideosFavoritos[i]);
                if (i < limite - 1) {
                    printf(", ");
                }
            }
            printf("\n");
        }
    }

    if (ferror(arq)) {
        printf("Erro durante a leitura do arquivo.\n");
    } else if (contador == 0) {
        printf("Nenhum usuario esta cadastrado!\n");
    }

    fclose(arq);
}

int RemoverVideoBinario(const char *nomeArquivo, int idRemover) {
    if (nomeArquivo == NULL) {
        return 0;
    }
    const char *nometemp = "temp_videos.bin";

    FILE *orig = fopen(nomeArquivo, "rb");
    if (orig == NULL) {
        printf("Erro ao abrir %s para remocao.\n", nomeArquivo);
        return 0;
    }
    if (!VerificarFormatoVideos(orig, nomeArquivo)) {
        fclose(orig);
        return 0;
    }

    FILE *temp = fopen(nometemp, "wb");
    if (temp == NULL) {
        printf("Erro ao criar arquivo temporario %s.\n", nometemp);
        fclose(orig);
        return 0;
    }

    struct VideoYoutube video;
    int encontrou = 0;
    int erroEscrita = 0;

    while (fread(&video, sizeof(struct VideoYoutube), 1, orig) == 1) {
        if (video.idVideo == idRemover) {
            encontrou = 1;
            continue;
        }

        if (fwrite(&video, sizeof(struct VideoYoutube), 1, temp) != 1) {
            erroEscrita = 1;
            break;
        }
    }

    if (ferror(orig)) {
        erroEscrita = 1;
    }

    int fechouOrig = fclose(orig);
    int fechoutemp = fclose(temp);

    if (erroEscrita || fechouOrig != 0 || fechoutemp != 0) {
        printf("Erro durante a remocao do video ID %d.\n", idRemover);
        remove(nometemp);
        return 0;
    }

    if (!encontrou) {
        remove(nometemp);
        return 0;
    }

    if (remove(nomeArquivo) != 0) {
        printf("Erro ao remover arquivo original %s.\n", nomeArquivo);
        remove(nometemp);
        return 0;
    }

    if (rename(nometemp, nomeArquivo) != 0) {
        printf("Erro ao renomear %s para %s.\n", nometemp, nomeArquivo);
        return 0;
    }

    return 1;
}

int BuscarVideoPorId(const char *nomeArquivo, int idBuscado, struct VideoYoutube *resultado) {
    if (nomeArquivo == NULL) {
        return 0;
    }

    FILE *arq = fopen(nomeArquivo, "rb");
    if (arq == NULL) {
        return 0;
    }

    if (!VerificarFormatoVideos(arq, nomeArquivo)) {
        fclose(arq);
        return 0;
    }

    struct VideoYoutube video;
    while (fread(&video, sizeof(struct VideoYoutube), 1, arq) == 1) {
        if (video.idVideo == idBuscado) {
            if (resultado != NULL) {
                *resultado = video;
            }
            fclose(arq);
            return 1;
        }
    }

    fclose(arq);
    return 0;
}

void ExibirVideosFavoritosUsuario(const struct Usuario *U, const char *nomeArquivoVideos) {
    if (U == NULL || nomeArquivoVideos == NULL) {
        printf("Parametros invalidos para exibir favoritos.\n");
        return;
    }

    printf("\n=== VIDEOS FAVORITOS DE %s (ID %d) ===\n", U->nome, U->idUsuario);

    if (U->qtdFavoritos <= 0) {
        printf("Este usuario nao possui videos favoritos cadastrados.\n");
        return;
    }

    int limite = U->qtdFavoritos;
    if (limite > TAM) {
        limite = TAM;
    }

    int exibidos = 0;

    for (int i = 0; i < limite; i++) {
        int idFav = U->idVideosFavoritos[i];
        struct VideoYoutube videoEncontrado;

        if (BuscarVideoPorId(nomeArquivoVideos, idFav, &videoEncontrado)) {
            printf("%d. [ID %d] %s\n", i + 1, videoEncontrado.idVideo, videoEncontrado.titulo);
            printf("   Link: %s\n", videoEncontrado.linkVideo);
            exibidos++;
        } else {
            printf("%d. [ID %d] Video nao encontrado em %s (pode ter sido removido).\n",
                   i + 1, idFav, nomeArquivoVideos);
        }
    }

    if (exibidos == 0) {
        printf("Nenhum dos IDs favoritos foi localizado no arquivo de videos.\n");
    }
}

int AtualizarVideoBinario(const char *nomeArquivo, struct VideoYoutube novoVideo) {
    FILE *arq = fopen(nomeArquivo, "r+b");
    if (arq == NULL) {
        printf("Erro ao abrir %s.\n", nomeArquivo);
        return 0;
    }
    if (!VerificarFormatoVideos(arq, nomeArquivo)) {
        fclose(arq);
        return 0;
    }

    struct VideoYoutube atual;
    while (fread(&atual, sizeof(struct VideoYoutube), 1, arq) == 1) {
        if (atual.idVideo == novoVideo.idVideo) {
            fseek(arq, -(long)sizeof(struct VideoYoutube), SEEK_CUR);
            int ok = (fwrite(&novoVideo, sizeof(struct VideoYoutube), 1, arq) == 1);
            fclose(arq);
            return ok;
        }
    }

    fclose(arq);
    return 0;
}

int AtualizarUsuarioBinario(const char *nomeArquivo, struct Usuario novoUsuario) {
    FILE *arq = fopen(nomeArquivo, "r+b");
    if (arq == NULL) {
        printf("Erro ao abrir %s.\n", nomeArquivo);
        return 0;
    }

    struct Usuario atual;
    while (fread(&atual, sizeof(struct Usuario), 1, arq) == 1) {
        if (atual.idUsuario == novoUsuario.idUsuario) {
            fseek(arq, -(long)sizeof(struct Usuario), SEEK_CUR);
            int ok = (fwrite(&novoUsuario, sizeof(struct Usuario), 1, arq) == 1);
            fclose(arq);
            return ok;
        }
    }

    fclose(arq);
    return 0;
}

void RelatorioUsuariosComFavoritos(const char *nomeArquivoUsuarios, const char *nomeArquivoVideos) {
    if (nomeArquivoUsuarios == NULL || nomeArquivoVideos == NULL) {
        printf("Parametros invalidos para gerar o relatorio.\n");
        return;
    }

    FILE *arq = fopen(nomeArquivoUsuarios, "rb");
    if (arq == NULL) {
        printf("Erro ao abrir %s para gerar o relatorio.\n", nomeArquivoUsuarios);
        return;
    }

    struct Usuario usuario;
    int contador = 0;

    printf("\n================ RELATORIO DE USUARIOS E VIDEOS FAVORITOS ================\n");

    while (fread(&usuario, sizeof(struct Usuario), 1, arq) == 1) {
        int limite = usuario.qtdFavoritos;
        if (limite < 0) limite = 0;
        if (limite > TAM) limite = TAM;

        printf("\nUsuario #%d | ID: %d | Nome: %s | Email: %s\n",
               ++contador, usuario.idUsuario, usuario.nome, usuario.email);

        if (limite == 0) {
            printf("  Videos favoritados: nenhum.\n");
            continue;
        }

        printf("  Videos favoritados:\n");
        for (int i = 0; i < limite; i++) {
            int idFav = usuario.idVideosFavoritos[i];
            struct VideoYoutube videoEncontrado;

            if (BuscarVideoPorId(nomeArquivoVideos, idFav, &videoEncontrado)) {
                printf("    - %s\n", videoEncontrado.titulo);
            } else {
                printf("    - [Video nao encontrado em %s]\n", nomeArquivoVideos);
            }
        }
    }

    if (ferror(arq)) {
        printf("\nErro durante a leitura do arquivo de usuarios.\n");
    } else if (contador == 0) {
        printf("\nNenhum usuario cadastrado para exibir no relatorio.\n");
    }

    fclose(arq);
}

static void LimparBufferEntrada(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
    }
}

static void LerLinha(char *destino, size_t tamanho) {
    if (destino == NULL || tamanho == 0) {
        return;
    }

    if (fgets(destino, (int)tamanho, stdin) != NULL) {
        size_t len = strlen(destino);
        if (len > 0 && destino[len - 1] == '\n') {
            destino[len - 1] = '\0';
        } else {
            LimparBufferEntrada();
        }
    } else {
        destino[0] = '\0';
    }
}

static void ExibirMenuPrincipal(void) {
    printf("\n=== SISTEMA DE VIDEOS E USUARIOS ===\n");
    printf("1 - Cadastrar video\n");
    printf("2 - Listar videos\n");
    printf("3 - Atualizar video\n");
    printf("4 - Remover video\n");
    printf("5 - Cadastrar usuario\n");
    printf("6 - Listar usuarios\n");
    printf("7 - Atualizar usuario\n");
    printf("8 - Exibir videos favoritos de um usuario\n");
    printf("9 - Relatorio de usuarios e titulos favoritados\n");
    printf("10 - Adicionar video aos favoritos\n");
    printf("11 - Remover video dos favoritos\n");
    printf("0 - Sair\n");
    printf("Escolha uma opcao: ");
}

static int LerOpcaoMenu(int *opcao) {
    if (opcao == NULL || scanf("%d", opcao) != 1) {
        LimparBufferEntrada();
        return 0;
    }
    LimparBufferEntrada();
    return 1;
}

static int BuscarUsuarioPorId(const char *nomeArquivo, int idBuscado, struct Usuario *resultado) {
    if (nomeArquivo == NULL) {
        return 0;
    }

    FILE *arq = fopen(nomeArquivo, "rb");
    if (arq == NULL) {
        return 0;
    }

    struct Usuario u;
    while (fread(&u, sizeof(struct Usuario), 1, arq) == 1) {
        if (u.idUsuario == idBuscado) {
            if (resultado != NULL) {
                *resultado = u;
            }
            fclose(arq);
            return 1;
        }
    }

    fclose(arq);
    return 0;
}

static void OpcaoCadastrarVideo(void) {
    int id;
    char titulo[100];
    char link[200];
    struct VideoYoutube video;

    printf("ID do video: ");
    if (scanf("%d", &id) != 1) {
        LimparBufferEntrada();
        printf("ID invalido.\n");
        return;
    }
    LimparBufferEntrada();

    printf("Titulo: ");
    LerLinha(titulo, sizeof(titulo));

    printf("Link do video: ");
    LerLinha(link, sizeof(link));

    if (CadastrarVideo(&video, id, titulo, link) && GravarVideoBinario(&video, ARQ_VIDEOS)) {
        printf("Video cadastrado e gravado com sucesso!\n");
    } else {
        printf("Falha ao cadastrar/gravar video.\n");
    }
}

static void OpcaoAtualizarVideo(void) {
    struct VideoYoutube novoVideo;

    printf("ID do video a atualizar: ");
    scanf("%d", &novoVideo.idVideo);
    LimparBufferEntrada();

    printf("Novo titulo: ");
    LerLinha(novoVideo.titulo, sizeof(novoVideo.titulo));

    printf("Novo link: ");
    LerLinha(novoVideo.linkVideo, sizeof(novoVideo.linkVideo));

    if (AtualizarVideoBinario(ARQ_VIDEOS, novoVideo)) {
        printf("Video atualizado com sucesso!\n");
    } else {
        printf("Video nao encontrado para atualizacao.\n");
    }
}

static void OpcaoRemoverVideo(void) {
    int idRemover;
    printf("ID do video a remover: ");
    scanf("%d", &idRemover);
    LimparBufferEntrada();

    if (RemoverVideoBinario(ARQ_VIDEOS, idRemover)) {
        printf("Video removido com sucesso!\n");
    } else {
        printf("Video nao encontrado ou erro na remocao.\n");
    }
}

static void OpcaoCadastrarUsuario(void) {
    int id, qtd = 0;
    char nome[100];
    char email[100];
    int favoritos[TAM];
    struct Usuario usuario;

    printf("ID do usuario: ");
    scanf("%d", &id);
    LimparBufferEntrada();

    printf("Nome: ");
    LerLinha(nome, sizeof(nome));

    printf("Email: ");
    LerLinha(email, sizeof(email));

    printf("Quantidade de videos favoritos (0 a %d): ", TAM);
    scanf("%d", &qtd);
    LimparBufferEntrada();

    if (qtd < 0) qtd = 0;
    if (qtd > TAM) qtd = TAM;

    for (int i = 0; i < qtd; i++) {
        printf("ID do favorito %d: ", i + 1);
        scanf("%d", &favoritos[i]);
        LimparBufferEntrada();
    }

    if (CadastrarUsuario(&usuario, id, nome, email, favoritos, qtd) &&
        GravarUsuarioBinario(&usuario, ARQ_USUARIOS)) {
        printf("Usuario cadastrado e gravado com sucesso!\n");
    } else {
        printf("Falha ao cadastrar/gravar usuario.\n");
    }
}

static void OpcaoAtualizarUsuario(void) {
    struct Usuario novoUsuario;

    printf("ID do usuario a atualizar: ");
    scanf("%d", &novoUsuario.idUsuario);
    LimparBufferEntrada();

    printf("Novo nome: ");
    LerLinha(novoUsuario.nome, sizeof(novoUsuario.nome));

    printf("Novo email: ");
    LerLinha(novoUsuario.email, sizeof(novoUsuario.email));

    printf("Nova quantidade de favoritos (0 a %d): ", TAM);
    scanf("%d", &novoUsuario.qtdFavoritos);
    LimparBufferEntrada();

    if (novoUsuario.qtdFavoritos < 0) novoUsuario.qtdFavoritos = 0;
    if (novoUsuario.qtdFavoritos > TAM) novoUsuario.qtdFavoritos = TAM;

    for (int i = 0; i < novoUsuario.qtdFavoritos; i++) {
        printf("ID do favorito %d: ", i + 1);
        scanf("%d", &novoUsuario.idVideosFavoritos[i]);
        LimparBufferEntrada();
    }

    if (AtualizarUsuarioBinario(ARQ_USUARIOS, novoUsuario)) {
        printf("Usuario atualizado com sucesso!\n");
    } else {
        printf("Usuario nao encontrado para atualizacao.\n");
    }
}

static void OpcaoExibirFavoritos(void) {
    int idUsuario;
    struct Usuario encontrado;

    printf("ID do usuario para ver favoritos: ");
    scanf("%d", &idUsuario);
    LimparBufferEntrada();

    if (BuscarUsuarioPorId(ARQ_USUARIOS, idUsuario, &encontrado)) {
        ExibirVideosFavoritosUsuario(&encontrado, ARQ_VIDEOS);
    } else {
        printf("Usuario com ID %d nao encontrado.\n", idUsuario);
    }
}

/* Atualiza somente o vetor de favoritos do registro do usuario em usuarios.bin. */
static int AlterarFavoritoUsuario(int idUsuario, int idVideo, int adicionar) {
    FILE *arq;
    struct Usuario usuario;
    int encontrado = 0;
    int i, pos = -1;

    if (adicionar) {
        struct VideoYoutube video;
        if (!BuscarVideoPorId(ARQ_VIDEOS, idVideo, &video)) {
            printf("Video ID %d nao encontrado em %s.\n", idVideo, ARQ_VIDEOS);
            return 0;
        }
    }

    arq = fopen(ARQ_USUARIOS, "r+b");
    if (arq == NULL) {
        perror("Erro ao abrir usuarios.bin");
        return 0;
    }

    while (fread(&usuario, sizeof(usuario), 1, arq) == 1) {
        if (usuario.idUsuario != idUsuario) continue;
        encontrado = 1;
        break;
    }
    if (!encontrado) {
        if (ferror(arq)) printf("Erro durante a leitura dos usuarios.\n");
        else printf("Usuario ID %d nao encontrado.\n", idUsuario);
        fclose(arq);
        return 0;
    }

    if (usuario.qtdFavoritos < 0 || usuario.qtdFavoritos > TAM) {
        printf("Quantidade de favoritos invalida no registro do usuario.\n");
        fclose(arq);
        return 0;
    }
    for (i = 0; i < usuario.qtdFavoritos; i++) {
        if (usuario.idVideosFavoritos[i] == idVideo) {
            pos = i;
            break;
        }
    }
    if (adicionar) {
        if (pos >= 0) {
            printf("Este video ja esta nos favoritos do usuario.\n");
            fclose(arq);
            return 0;
        }
        if (usuario.qtdFavoritos >= TAM) {
            printf("Limite de %d favoritos atingido.\n", TAM);
            fclose(arq);
            return 0;
        }
        usuario.idVideosFavoritos[usuario.qtdFavoritos++] = idVideo;
    } else {
        if (pos < 0) {
            printf("Este video nao esta nos favoritos do usuario.\n");
            fclose(arq);
            return 0;
        }
        for (i = pos; i < usuario.qtdFavoritos - 1; i++) {
            usuario.idVideosFavoritos[i] = usuario.idVideosFavoritos[i + 1];
        }
        usuario.qtdFavoritos--;
        usuario.idVideosFavoritos[usuario.qtdFavoritos] = 0;
    }

    if (fseek(arq, -(long)sizeof(usuario), SEEK_CUR) != 0 ||
        fwrite(&usuario, sizeof(usuario), 1, arq) != 1) {
        perror("Erro ao atualizar favoritos");
        fclose(arq);
        return 0;
    }
    if (fclose(arq) != 0) {
        perror("Erro ao finalizar atualizacao dos favoritos");
        return 0;
    }
    return 1;
}

static void OpcaoAdicionarFavorito(void) {
    int idUsuario, idVideo;
    printf("ID do usuario: ");
    if (scanf("%d", &idUsuario) != 1) {
        LimparBufferEntrada();
        printf("ID de usuario invalido.\n");
        return;
    }
    LimparBufferEntrada();
    printf("ID do video para adicionar: ");
    if (scanf("%d", &idVideo) != 1) {
        LimparBufferEntrada();
        printf("ID de video invalido.\n");
        return;
    }
    LimparBufferEntrada();
    if (AlterarFavoritoUsuario(idUsuario, idVideo, 1)) {
        printf("Video adicionado aos favoritos com sucesso!\n");
    }
}

static void OpcaoRemoverFavorito(void) {
    int idUsuario, idVideo;
    printf("ID do usuario: ");
    if (scanf("%d", &idUsuario) != 1) {
        LimparBufferEntrada();
        printf("ID de usuario invalido.\n");
        return;
    }
    LimparBufferEntrada();
    printf("ID do video para remover: ");
    if (scanf("%d", &idVideo) != 1) {
        LimparBufferEntrada();
        printf("ID de video invalido.\n");
        return;
    }
    LimparBufferEntrada();
    if (AlterarFavoritoUsuario(idUsuario, idVideo, 0)) {
        printf("Video removido dos favoritos com sucesso!\n");
    }
}

int main(void) {
    int opcao = -1;

    do {
        ExibirMenuPrincipal();

        if (!LerOpcaoMenu(&opcao)) {
            printf("EI, DIGITA UM NUMERO DIREITO!\n");
            opcao = -1;
            continue;
        }

        switch (opcao) {
            case 1: OpcaoCadastrarVideo(); break;
            case 2: ListarVideos(ARQ_VIDEOS); break;
            case 3: OpcaoAtualizarVideo(); break;
            case 4: OpcaoRemoverVideo(); break;
            case 5: OpcaoCadastrarUsuario(); break;
            case 6: ListarUsuarios(ARQ_USUARIOS); break;
            case 7: OpcaoAtualizarUsuario(); break;
            case 8: OpcaoExibirFavoritos(); break;
            case 9: RelatorioUsuariosComFavoritos(ARQ_USUARIOS, ARQ_VIDEOS); break;
            case 10: OpcaoAdicionarFavorito(); break;
            case 11: OpcaoRemoverFavorito(); break;
            case 0: printf("TCHAUZINHO, ARQUIVOS SALVOS!\n"); break;
            default: printf("OPCAO INVESÍVEL! TENTA DE NOVO!\n"); break;
        }
    } while (opcao != 0);

    return 0;
}