# Format OBP (OBJ with Pivots)

Format simple style OBJ avec support des bones/pivots pour animations.

## Extension
`.obp` (OBJ Pivot)

## Spécification

### Commandes de base (compatibles OBJ)
```
v x y z              # Vertex (point 3D)
vt u v               # Texture coordinate (UV)
vn x y z             # Normal (optionnel)
f v1/vt1 v2/vt2 ...  # Face (triangle/quad)
```

### Commandes pour bones
```
b name px py pz      # Bone (nom + pivot x y z)
                     # Tous les v/vt/f suivants appartiennent à ce bone
                     # Jusqu'au prochain 'b' ou fin de fichier
```

### Commandes pour animations (optionnel)
```
anim name length loop    # Animation (length en secondes, loop = 0 ou 1)
key time bone rx ry rz   # Keyframe de rotation (temps en secondes, angles en degrés)
```

## Exemple complet

```obp
# Modèle simple avec un cube
b head 0.0 24.0 0.0
v 0.0 24.0 0.0
v 8.0 24.0 0.0
v 8.0 32.0 0.0
v 0.0 32.0 0.0
v 0.0 24.0 8.0
v 8.0 24.0 8.0
v 8.0 32.0 8.0
v 0.0 32.0 8.0
vt 0.0 0.0
vt 1.0 0.0
vt 1.0 1.0
vt 0.0 1.0
f 1/1 2/2 3/3 4/4
f 6/1 5/2 8/3 7/4
f 5/1 6/2 2/3 1/4
f 4/1 3/2 7/3 6/4
f 2/1 6/2 7/3 3/4
f 5/1 1/2 4/3 8/4

# Animation
anim idle 2.25 1
key 0.0 head 0.0 0.0 0.0
key 1.0 head 25.0 0.0 0.0
key 2.25 head 0.0 0.0 0.0
```

## Avantages
- ✅ Simple et lisible (comme OBJ)
- ✅ Support des bones/pivots
- ✅ Support des animations
- ✅ Facile à parser
- ✅ Pas de base64 ou binaire
- ✅ Éditable à la main

## Structure hiérarchique
Pour des bones imbriqués (parent/enfant) :
```
b body 0.0 0.0 0.0
  b **Ultra simple** - Format minimaliste
- ✅ **Lisible** - Texte pur, comme OBJ
- ✅ **Bones avec pivots** - Pour animations
- ✅ **Animations intégrées** - Dans le même fichier
- ✅ **Facile à parser** - Ligne par ligne
- ✅ **Pas de binaire** - Pas de base64
- ✅ **Éditable à la main** - Avec n'importe quel éditeur texte

## Multi-bones
Plusieurs bones dans un modèle :
```obp
b body 0.0 0.0 0.0
v -4 0 -4
v 4 0 -4
# ... vertices du body
f 1/1 2/2 3/3 4/4

b head 0.0 24.0 0.0
v 0 24 0
v 8 24 0
# ... vertices de la tête
f 9/1 10/2 11/3 12/4