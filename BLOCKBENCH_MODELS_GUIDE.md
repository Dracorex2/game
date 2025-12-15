# Guide des 3 Types de Modèles Blockbench

Le système `tile_texture` a été retiré. Voici les 3 types de modèles optimisés à utiliser:

---

## 1. CUBE SIMPLE (1 texture répétée)

**Fichier**: `models/cube_simple.json`  
**Texture**: 16x16 pixels  
**Usage**: Blocs avec la même texture sur toutes les faces (stone, dirt, ore, etc.)

### Configuration Blockbench:
- **Texture Width**: 16
- **Texture Height**: 16
- **UV Start**: [0, 0]

### Layout de texture (16x16):
```
┌────────┐
│        │  Texture unique qui sera
│ TEXTURE│  répétée sur TOUTES les faces
│        │  (top, bottom, north, south, east, west)
└────────┘
```

### Template Texture:
Créez simplement une image 16x16 avec votre motif (stone, dirt, etc.)

**Exemple d'utilisation**:
```
blocks.block:
Stone 1 models/cube_simple.json textures/stone_16x16.png
Dirt  2 models/cube_simple.json textures/dirt_16x16.png
Ore   3 models/cube_simple.json textures/ore_16x16.png
```

---

## 2. CUBE MINECRAFT (côtés + top/bottom)

**Fichier**: `models/cube_minecraft.json`  
**Texture**: 48x32 pixels  
**Usage**: Style Minecraft classique (grass, log, etc.) - texture différente pour top/bottom vs côtés

### Configuration Blockbench:
- **Texture Width**: 48
- **Texture Height**: 32
- **UV Start**: [0, 16]

### Layout de texture (48x32):
```
┌────┬────┬────┐
│TOP │SIDE│SIDE│  ← Ligne du haut (Y: 0-16)
├────┼────┼────┤
│SIDE│SIDE│BOTT│  ← Ligne du bas (Y: 16-32)
└────┴────┴────┘
 0   16   32  48

Détail du mapping Bedrock Box UV avec uv=[0,16]:
- Top:    X: 0-16,   Y: 0-16
- North:  X: 16-32,  Y: 16-32 (face avant)
- South:  X: 32-48,  Y: 16-32 (face arrière)
- West:   X: 0-16,   Y: 16-32 (face gauche)
- East:   X: 32-48,  Y: 16-32 (face droite)
- Bottom: X: 16-32,  Y: 0-16
```

### Template Texture (grass block):
```
┌─────────┬─────────┬─────────┐
│  GRASS  │  DIRT   │  DIRT   │  ← Ligne du haut
│   TOP   │  SIDE   │  SIDE   │
├─────────┼─────────┼─────────┤
│  DIRT   │  DIRT   │  DIRT   │  ← Ligne du bas
│  SIDE   │  SIDE   │  BOTTOM │
└─────────┴─────────┴─────────┘
```

**Exemple d'utilisation**:
```
blocks.block:
Grass 2 models/cube_minecraft.json textures/grass_48x32.png
Log   6 models/cube_minecraft.json textures/log_48x32.png
```

---

## 3. CUBE COMPLET (6 faces distinctes)

**Fichier**: `models/cube_full.json`  
**Texture**: 64x32 pixels  
**Usage**: Modèles complexes avec une texture différente par face (crafting table, furnace, mob heads)

### Configuration Blockbench:
- **Texture Width**: 64
- **Texture Height**: 32
- **UV Start**: [0, 16]

### Layout de texture (64x32):
```
┌────┬────┬────┬────┐
│TOP │    │    │    │  ← Ligne du haut (Y: 0-16)
├────┼────┼────┼────┤
│WEST│FRNT│EAST│BACK│  ← Ligne du bas (Y: 16-32)
└────┴────┴────┴────┘
 0   16   32   48  64
│BOTT│

Détail du mapping Bedrock Box UV complet avec uv=[0,16]:
- Top:    X: 0-16,   Y: 0-16
- North:  X: 16-32,  Y: 16-32 (face avant - FRONT)
- South:  X: 48-64,  Y: 16-32 (face arrière - BACK)
- West:   X: 0-16,   Y: 16-32 (face gauche - LEFT)
- East:   X: 32-48,  Y: 16-32 (face droite - RIGHT)
- Bottom: X: 16-32,  Y: 0-16
```

### Template Texture (crafting table):
```
┌──────┬──────┬──────┬──────┐
│CRAFT │      │      │      │  ← Ligne du haut
│ TOP  │      │      │      │
├──────┼──────┼──────┼──────┤
│PLANK │CRAFT │PLANK │PLANK │  ← Ligne du bas
│ SIDE │FRONT │ SIDE │ BACK │
└──────┴──────┴──────┴──────┘
│PLANK │
│BOTTOM│
```

**Exemple d'utilisation**:
```
blocks.block:
CraftingTable 10 models/cube_full.json textures/crafting_64x32.png
Furnace       11 models/cube_full.json textures/furnace_64x32.png
```

---

## Règles du Box UV Mapping (Bedrock)

Pour un cube de taille [16, 16, 16] avec `uv=[u, v]`:

```c
// Variables:
// u, v = Position UV de départ
// w = largeur du cube (16)
// h = hauteur du cube (16)
// d = profondeur du cube (16)

Face North (Front):  uv = [u+d+w, v+d] à [u+d, v+d+h]
Face South (Back):   uv = [u+d+w+d, v+d] à [u+d+w+d+w, v+d+h]
Face West (Left):    uv = [u+d, v+d] à [u, v+d+h]
Face East (Right):   uv = [u+d+w+d, v+d] à [u+d+w, v+d+h]
Face Top:            uv = [u+d, v] à [u+d+w, v+d]
Face Bottom:         uv = [u+d+w, v] à [u+d+w+w, v+d]
```

---

## Quelle modèle choisir?

| Type | Résolution | Cas d'usage |
|------|-----------|-------------|
| **Simple** | 16x16 | Blocs homogènes (stone, dirt, ore, wool) |
| **Minecraft** | 48x32 | Top/Bottom différent des côtés (grass, log, sandstone) |
| **Full** | 64x32 | Chaque face unique (crafting table, furnace, heads) |

---

## Workflow Blockbench

### Pour créer un nouveau modèle:

1. **File → New → Bedrock Model**
2. Paramètres:
   - **Model Identifier**: geometry.your_block
   - **Texture Width/Height**: Selon le type choisi
   - **Box UV**: ✅ Coché
3. Créez votre cube (16x16x16 pour un bloc standard)
4. **Paint → Create Template** pour voir le layout UV
5. Exportez le template et peignez votre texture dans un éditeur d'image
6. Importez votre texture finie dans Blockbench
7. **File → Export → Export Bedrock Geometry**

### Options d'export de texture:
- ✅ **Power of 2 Size**: TOUJOURS cocher
- ❌ **Rearrange UV**: Décocher (gardez le layout box UV)
- ⚠️ **Keep Opacity**: Cocher si transparent (glass, leaves)
- **Padding**: 0 (pas nécessaire avec box UV)

---

## Exemples de Textures

### Stone (Simple - 16x16):
```
Une seule image 16x16 avec le motif pierre
```

### Grass (Minecraft - 48x32):
```
┌───────┬───────┬───────┐
│ GRASS │ TERRE │ TERRE │
│  VERT │ BRUNE │ BRUNE │
├───────┼───────┼───────┤
│ TERRE │ TERRE │ TERRE │
│ BRUNE │ BRUNE │ BRUNE │
└───────┴───────┴───────┘
```

### Crafting Table (Full - 64x32):
```
┌──────┬──────┬──────┬──────┐
│GRILLE│      │      │      │
│CRAFT │      │      │      │
├──────┼──────┼──────┼──────┤
│PLANCHE│GRILLE│PLANCHE│PLANCHE│
│      │+OUTIL│      │      │
└──────┴──────┴──────┴──────┘
│PLANCHE│
```

---

## Migration depuis l'ancien système

**Avant** (avec tile_texture):
```json
{
  "description": {
    "tile_texture": true
  }
}
```

**Maintenant** (sans tile_texture):
- Utilisez `cube_simple.json` pour le même résultat
- Ou créez votre propre layout UV avec cube_minecraft/cube_full

---

## Optimisations

### Mémoire RAM/VRAM:
- Simple (16x16): ~1 KB par texture
- Minecraft (48x32): ~6 KB par texture
- Full (64x32): ~8 KB par texture

### Performance:
Tous les types ont **exactement la même performance** - ils utilisent tous le même code de rendu Box UV!

La seule différence est la **taille de texture** et l'**organisation des UV**.
