#ifndef TYPES_H
#define TYPES_H

#include <pthread.h>
#include <stdio.h>
#include "obp_loader.h"

#define CHUNK_SIZE_X 16
#define CHUNK_SIZE_Y 16
#define CHUNK_SIZE_Z 16

#define WORLD_CHUNKS_X 20
#define WORLD_CHUNKS_Z 20

#define CAM_WIDTH 0.5f
#define CAM_HEIGHT 1.8f
#define EYE_HEIGHT 1.6f

// Vector indices
#define X 0
#define Y 1
#define Z 2

// Block ID type
typedef int BlockType;

#define BLOCK_AIR 0

// Forward declarations
typedef struct BlockDefinition BlockDefinition;
typedef struct RenderThreadState RenderThreadState;
typedef struct Chunk Chunk;
typedef struct TileEntity TileEntity;

// Définition du pointeur de fonction pour le rendu d'entité
// modelMatrix contient déjà la translation (x,y,z) et la rotation de base (N/S/E/W)
typedef void (*EntityRenderFunc)(TileEntity* te, BlockDefinition* def, unsigned int shader, float* modelMatrix);

// Block structure (instance d'un bloc dans le monde)
typedef struct {
    BlockType type; 
} Block;

// Tile Entity (Bloc spécial avec données dynamiques : coffre, four, etc.)
struct TileEntity {
    int x, y, z;        // Position locale dans le chunk (0-15)
    BlockType type;     // Type de bloc
    float animState;    // État d'animation (ex: 0.0 fermé -> 1.0 ouvert)
    int rotation;       // Orientation (0=Nord, 1=Est, 2=Sud, 3=Ouest)
    // On pourra ajouter ici des données spécifiques (inventaire, fuel, etc.) via une union
};


// Chunk structure
struct Chunk {
    Block blocks[CHUNK_SIZE_X][CHUNK_SIZE_Y][CHUNK_SIZE_Z];
    
    // Mesh statique (optimisé)
    unsigned int VAO, VBO;
    unsigned int transparentVAO, transparentVBO;
    unsigned int foliageVAO, foliageVBO;
    int vertexCount;
    int transparentVertexCount;
    int foliageVertexCount;
    
    // Entités dynamiques (rendues séparément)
    TileEntity* tileEntities;
    int tileEntityCount;
    int tileEntityCapacity;
    
    int needsRebuild;
};

// Render thread state
struct RenderThreadState {
    pthread_t thread;
    pthread_mutex_t mutex;
    int running;
    int shouldExit;
    
    // Données de rendu (protégées par mutex)
    float viewMatrix[16];
    float projectionMatrix[16];
    float cameraX, cameraY, cameraZ;
    int framebufferWidth, framebufferHeight;
    int selectedBlockID;
};

// Options de jeu configurables
typedef struct {
    float renderDistance;      // Distance de rendu en chunks (ex: 12.0 = ~192 blocs)
    float fov;                 // Champ de vision en degrés (ex: 60.0)
    int vsync;                 // VSync activé (1) ou désactivé (0)
    int showFps;               // Afficher les FPS (1) ou non (0)
} GameOptions;

// Structure globale du jeu - contient toutes les variables importantes
typedef struct {
    // === BLOCS ===
    BlockDefinition* blocks;     // Tableau de définitions de blocs
    int blockCount;              // Nombre de blocs chargés
    
    // === MONDE ===
    Chunk** world;               // Grille de chunks
    unsigned int textureAtlas;   // ID de la texture atlas
    int atlasMaxFrames;          // Nombre max de frames d'animation
    
    // === CAMÉRA ===
    float plPos[3];              // Position du joueur (anciennement cameraPos)
    float cameraFront[3];        // Direction de la caméra
    float cameraUp[3];           // Vecteur up de la caméra
    float yaw;                   // Rotation horizontale
    float pitch;                 // Rotation verticale
    float velocityY;             // Vélocité verticale (gravité)
    
    // === TIMING ===
    float deltaTime;             // Temps écoulé depuis la dernière frame
    float lastFrame;             // Timestamp de la dernière frame
    float currentFrameTime;      // Temps actuel précalculé (optimisation pour TileEntities)
    
    // === OPTIONS ===
    GameOptions options;         // Options de jeu configurables
    
    // === ETAT JEU ===
    int selectedBlockID;         // ID du bloc actuellement sélectionné pour le placement
    
    // === RENDU ===
    RenderThreadState* renderThread;  // Thread de rendu
} GameContext;

// Block definition (toutes les propriétés d'un type de bloc)
struct BlockDefinition {
    BlockType id;
    char* name;              // Alloué dynamiquement
    EntityRenderFunc renderFunc; // Fonction de rendu spécifique
    unsigned int textureID;
    int solid;       // 1 = collision, 0 = traversable
    int transparent; // 1 = voir à travers (feuilles), 0 = opaque
    int translucent; // 1 = translucide (verre, eau), ne rend pas faces internes
    int isDynamic;   // 1 = rendu via drawTileEntities (animé), 0 = rendu statique dans le chunk
    int animFrames;  // Nombre de frames d'animation (1 = statique)
    OBPModel* model; // Modèle OBP chargé
    
    // Données de texture préchargées (temporaire avant création atlas)
    unsigned char* pixelData;
    unsigned texWidth;
    unsigned texHeight;
};

// Block texture structure
typedef struct {
    const char* texturePath;
    unsigned int textureID;
} BlockTexture;

// Instance globale du contexte de jeu
extern GameContext game;

#endif
