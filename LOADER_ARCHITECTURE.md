# Bedrock Loader Architecture

## Overview
The Bedrock JSON model loader has been split into two specialized loaders to separate concerns:

- **Block Loader** (`bedrock_block_loader.c/h`) - For simple static geometry
- **Entity Loader** (`bedrock_entity_loader.c/h`) - For complex animated models

Both loaders use the same underlying Bedrock JSON parser (`bedrock_loader.c/h`).

---

## Block Loader (`bedrock_block_loader`)

**Purpose**: Load simple block models that are baked into static chunk meshes.

**File**: `src/bedrock_block_loader.c`, `include/bedrock_block_loader.h`

**Function**: 
```c
BedrockModel* loadBedrockBlockModel(const char* filepath, int uploadToGPU, int texW, int texH);
```

**Characteristics**:
- Validates model complexity (warns if >10 bones)
- Used for blocks defined in `blocks.block`
- Models are baked into static chunk meshes for performance
- Simple texture animation only (fire, water)
- No 3D skeletal animation

**Examples**:
- Stone, Dirt, Grass - simple cubes
- Flowers, Torches - decorative elements
- Water, Lava - texture-animated blocks
- Glass, Leaves - transparent/translucent blocks

**Configuration file**: `blocks.block`
```
Stone 1 models/test_cube.json textures/stone.png
Grass 2 models/test_cube.json textures/grass.png
Flower 4 models/flower.json textures/flower_animated.png 32
```

---

## Entity Loader (`bedrock_entity_loader`)

**Purpose**: Load complex entity models with 3D animations, state, and dynamic behavior.

**File**: `src/bedrock_entity_loader.c`, `include/bedrock_entity_loader.h`

**Functions**:
```c
BedrockModel* loadBedrockEntityModel(const char* filepath, int uploadToGPU, int texW, int texH);
Animation* loadBedrockAnimation(const char* filepath);
```

**Characteristics**:
- Supports complex bone hierarchies (no limit)
- Used for entities defined in `entities.block` (formerly `tile_entities.block`)
- Models rendered dynamically per-entity
- Supports 3D skeletal animations (keyframes, bone rotations)
- Can have state (open/closed, active/inactive)
- Can have orientation (facing N/S/E/W)
- Can have inventories and custom data

**Examples**:
- Chest - opens/closes with animation
- Hopper - item collection mechanics
- Furnace - active/inactive states
- Door - opening animation with state
- Mobs/Animals - complex multi-bone animations

**Configuration file**: `entities.block` (or `tile_entities.block`)
```
Chest 10 models/chest.json textures/chest.png renderChest
@ animations/chest.json
Hopper 11 models/hopper.json textures/hopper.png renderHopper
```

---

## Core Bedrock Loader (`bedrock_loader`)

**Purpose**: Low-level Bedrock JSON parser used by both specialized loaders.

**File**: `src/bedrock_loader.c`, `include/bedrock_loader.h`

**Function**:
```c
BedrockModel* loadBedrockModel(const char* path, int isBlock, int texW, int texH);
```

**Features**:
- Parses Minecraft Bedrock Edition `.geo.json` format
- Supports bones, cubes, pivots, rotations, UV mapping
- Compatible with Blockbench exports
- Handles `tile_texture` flag for automatic UV tiling
- Builds GPU buffers for rendering

**Should not be called directly** - Use `loadBedrockBlockModel` or `loadBedrockEntityModel` instead.

---

## Usage Guidelines

### When to use Block Loader (blocks.block)
✅ Static decorative elements  
✅ Terrain blocks (stone, dirt, grass)  
✅ Simple texture animations only  
✅ No 3D skeletal animation needed  
✅ Baked into chunk mesh for performance  

### When to use Entity Loader (entities.block)
✅ Opening/closing animations (chest, door)  
✅ Multi-bone skeletal animations  
✅ State management (open/closed, active/inactive)  
✅ Orientation/rotation (N/S/E/W facing)  
✅ Inventories or custom data storage  
✅ Mobs, animals, NPCs  

---

## File Structure

```
include/
├── bedrock_loader.h           # Core parser (don't call directly)
├── bedrock_block_loader.h     # Block loader wrapper
└── bedrock_entity_loader.h    # Entity loader wrapper

src/
├── bedrock_loader.c           # Core parser implementation
├── bedrock_block_loader.c     # Block loader with validation
├── bedrock_entity_loader.c    # Entity loader with animation support
├── blockloader.c              # Reads blocks.block, uses bedrock_block_loader
└── entityloader.c             # Reads entities.block, uses bedrock_entity_loader
```

---

## Migration Notes

**Old code**:
```c
#include "bedrock_loader.h"
BedrockModel* model = loadBedrockModel(path, 1, 0, 0);
```

**New code for blocks**:
```c
#include "bedrock_block_loader.h"
BedrockModel* model = loadBedrockBlockModel(path, 1, 0, 0);
```

**New code for entities**:
```c
#include "bedrock_entity_loader.h"
BedrockModel* model = loadBedrockEntityModel(path, 1, 0, 0);
Animation* anim = loadBedrockAnimation("animations/my_anim.json");
```

---

## Technical Details

### Validation
- **Block Loader**: Warns if model has >10 bones (suggests using entity loader)
- **Entity Loader**: No bone limit, designed for complex models

### Performance
- **Blocks**: Baked into static chunk meshes → extremely fast rendering
- **Entities**: Rendered individually → slower but allows animation/state

### Animation Support
- **Blocks**: Texture animation only (multiple frames in texture file)
- **Entities**: Full 3D skeletal animation with keyframes and bone transforms

---

## Compilation
Both loaders are automatically compiled and linked by the Makefile.

The split architecture is backward compatible - existing models work without changes.
