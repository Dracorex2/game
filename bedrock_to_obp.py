#!/usr/bin/env python3
"""
Convertisseur Bedrock JSON → OBP (OBJ with Pivots)
Convertit un modèle Minecraft Bedrock et ses animations en format OBP
"""

import json
import sys
import math
from pathlib import Path


def rotate_point(point, pivot, rotation):
    """Rotate a point around a pivot by given angles (in degrees)"""
    px, py, pz = point
    cx, cy, cz = pivot
    rx, ry, rz = rotation
    
    # Convert to radians
    rad_x = math.radians(rx)
    rad_y = math.radians(ry)
    rad_z = math.radians(rz)
    
    # Translate to origin relative to pivot
    x = px - cx
    y = py - cy
    z = pz - cz
    
    # Rotate around X
    y_new = y * math.cos(rad_x) - z * math.sin(rad_x)
    z_new = y * math.sin(rad_x) + z * math.cos(rad_x)
    y = y_new
    z = z_new
    
    # Rotate around Y
    x_new = x * math.cos(rad_y) + z * math.sin(rad_y)
    z_new = -x * math.sin(rad_y) + z * math.cos(rad_y)
    x = x_new
    z = z_new
    
    # Rotate around Z
    x_new = x * math.cos(rad_z) - y * math.sin(rad_z)
    y_new = x * math.sin(rad_z) + y * math.cos(rad_z)
    x = x_new
    y = y_new
    
    # Translate back
    return (x + cx, y + cy, z + cz)


def generate_cube_vertices(origin, size, pivot=None, rotation=None):
    """Génère les 8 vertices d'un cube, avec rotation optionnelle"""
    ox, oy, oz = origin
    sx, sy, sz = size
    
    vertices = [
        (ox, oy, oz),           # 0: front bottom left
        (ox + sx, oy, oz),      # 1: front bottom right
        (ox + sx, oy + sy, oz), # 2: front top right
        (ox, oy + sy, oz),      # 3: front top left
        (ox, oy, oz + sz),      # 4: back bottom left
        (ox + sx, oy, oz + sz), # 5: back bottom right
        (ox + sx, oy + sy, oz + sz), # 6: back top right
        (ox, oy + sy, oz + sz)  # 7: back top left
    ]
    
    # Appliquer la rotation si nécessaire
    if rotation and pivot and any(r != 0 for r in rotation):
        rotated_vertices = []
        for v in vertices:
            rotated_vertices.append(rotate_point(v, pivot, rotation))
        return rotated_vertices
    
    return vertices


def generate_cube_faces(vertex_offset, size):
    """Génère les faces d'un cube, en évitant les doublons si épaisseur nulle"""
    # Format: [v1, v2, v3, v4] pour chaque face
    faces = []
    
    sx, sy, sz = size
    
    # Front face (Z) - Toujours générer
    faces.append([vertex_offset + 0, vertex_offset + 1, vertex_offset + 2, vertex_offset + 3])
    
    # Back face (Z) - Seulement si épaisseur Z > 0
    if sz > 0:
        faces.append([vertex_offset + 5, vertex_offset + 4, vertex_offset + 7, vertex_offset + 6])
        
    # Bottom face (Y) - Seulement si hauteur Y > 0
    if sy > 0:
        faces.append([vertex_offset + 4, vertex_offset + 5, vertex_offset + 1, vertex_offset + 0])
        
    # Top face (Y) - Seulement si hauteur Y > 0
    if sy > 0:
        faces.append([vertex_offset + 3, vertex_offset + 2, vertex_offset + 6, vertex_offset + 7])
        
    # Right face (X) - Seulement si largeur X > 0
    if sx > 0:
        faces.append([vertex_offset + 1, vertex_offset + 5, vertex_offset + 6, vertex_offset + 2])
        
    # Left face (X) - Seulement si largeur X > 0
    if sx > 0:
        faces.append([vertex_offset + 4, vertex_offset + 0, vertex_offset + 3, vertex_offset + 7])
    
    return faces


def get_uv_rect(face_name, uv_origin, size, tex_width, tex_height):
    """Calcule les coordonnées UV (u_min, v_min, u_max, v_max) pour une face donnée (Box UV)"""
    u, v = uv_origin
    sx, sy, sz = size
    
    # Mapping standard Bedrock (Box UV)
    # x=width(sx), y=height(sy), z=depth(sz)
    
    # Définition des rectangles [u, v, w, h] dans la texture
    # Attention: Bedrock (0,0) est Top-Left, OBJ (0,0) est Bottom-Left
    
    rect = [0, 0, 0, 0]
    
    if face_name == 'north': # Front (Z)
        rect = [u + sz, v + sz, sx, sy]
    elif face_name == 'south': # Back (Z)
        rect = [u + sz + sx + sz, v + sz, sx, sy]
    elif face_name == 'east': # Right (X)
        rect = [u, v + sz, sz, sy]
    elif face_name == 'west': # Left (X)
        rect = [u + sz + sx, v + sz, sz, sy]
    elif face_name == 'up': # Top (Y)
        rect = [u + sz, v, sx, sz]
    elif face_name == 'down': # Bottom (Y)
        rect = [u + sz + sx, v, sx, sz]
        
    # Conversion en coordonnées normalisées (0.0 - 1.0)
    # u_min, v_min, u_max, v_max
    # v est inversé pour OBJ (1.0 - v)
    
    u_min = rect[0] / tex_width
    v_max = 1.0 - (rect[1] / tex_height) # Top of texture rect (low v in bedrock)
    u_max = (rect[0] + rect[2]) / tex_width
    v_min = 1.0 - ((rect[1] + rect[3]) / tex_height) # Bottom of texture rect (high v in bedrock)
    
    return (u_min, v_min, u_max, v_max)


def convert_bedrock_to_obp(model_path, animation_path=None, output_path=None):
    """Convertit un modèle Bedrock JSON en format OBP"""
    
    # Charger le modèle
    with open(model_path, 'r') as f:
        model_data = json.load(f)
    
    # Charger l'animation si fournie
    animation_data = None
    if animation_path:
        with open(animation_path, 'r') as f:
            animation_data = json.load(f)
    
    # Déterminer le chemin de sortie
    if output_path is None:
        output_path = Path(model_path).with_suffix('.obp')
    
    # Écrire le fichier OBP
    with open(output_path, 'w') as f:
        f.write("# Converted from Bedrock JSON\n")
        f.write(f"# Source: {model_path}\n\n")
        
        # Parser le modèle Bedrock
        geometry = None
        if "minecraft:geometry" in model_data:
            geometry = model_data["minecraft:geometry"][0]
        elif "geometry" in model_data:
            geometry = model_data["geometry"]
        else:
            print("Erreur: Format Bedrock non reconnu")
            return
            
        # Récupérer la taille de texture
        description = geometry.get("description", {})
        tex_width = description.get("texture_width", 64)
        tex_height = description.get("texture_height", 64)
        
        bones = geometry.get("bones", [])
        
        vertex_count = 1  # OBJ commence à 1, pas 0
        uv_count = 0  # Compteur de coordonnées UV
        
        # Traiter chaque bone
        for bone in bones:
            bone_name = bone.get("name", "unnamed")
            pivot = bone.get("pivot", [0, 0, 0])
            
            # Écrire le bone
            f.write(f"b {bone_name} {pivot[0]} {pivot[1]} {pivot[2]}\n")
            
            # Traiter les cubes de ce bone
            cubes = bone.get("cubes", [])
            
            for cube in cubes:
                origin = cube.get("origin", [0, 0, 0])
                size = cube.get("size", [1, 1, 1])
                pivot = cube.get("pivot", [0, 0, 0])
                rotation = cube.get("rotation", [0, 0, 0])
                uv_origin = cube.get("uv", [0, 0])
                
                # Générer les vertices
                vertices = generate_cube_vertices(origin, size, pivot, rotation)
                for v in vertices:
                    f.write(f"v {v[0]} {v[1]} {v[2]}\n")
                
                # Identifier les faces actives (ordre de generate_cube_faces)
                # 1. Front (North)
                # 2. Back (South)
                # 3. Bottom (Down)
                # 4. Top (Up)
                # 5. Right (East)
                # 6. Left (West)
                
                active_faces = []
                sx, sy, sz = size
                
                active_faces.append('north')
                if sz > 0: active_faces.append('south')
                if sy > 0: active_faces.append('down')
                if sy > 0: active_faces.append('up')
                if sx > 0: active_faces.append('east')
                if sx > 0: active_faces.append('west')
                
                # Générer les UVs pour chaque face active
                uv_base = uv_count + 1
                
                for face_name in active_faces:
                    # Si uv_origin est une liste [u, v], utiliser Box UV
                    if isinstance(uv_origin, list):
                        u_min, v_min, u_max, v_max = get_uv_rect(face_name, uv_origin, size, tex_width, tex_height)
                        
                        f.write(f"vt {u_min} {v_min}\n")
                        f.write(f"vt {u_max} {v_min}\n")
                        f.write(f"vt {u_max} {v_max}\n")
                        f.write(f"vt {u_min} {v_max}\n")
                        
                    # Si uv_origin est un dictionnaire, utiliser Per-Face UV
                    elif isinstance(uv_origin, dict):
                        # Format: "north": {"uv": [0, 0], "uv_size": [16, 16]}
                        face_data = uv_origin.get(face_name)
                        
                        if face_data:
                            uv = face_data.get("uv", [0, 0])
                            uv_size = face_data.get("uv_size", None)
                            
                            # Si uv_size n'est pas spécifié, utiliser la taille de la face
                            if uv_size is None:
                                sx, sy, sz = size
                                if face_name in ['north', 'south']: uv_size = [sx, sy]
                                elif face_name in ['east', 'west']: uv_size = [sz, sy]
                                elif face_name in ['up', 'down']:   uv_size = [sx, sz]
                            
                            u, v = uv
                            w, h = uv_size
                            
                            # Calculer les coordonnées normalisées
                            u_min = u / tex_width
                            v_max = 1.0 - (v / tex_height)
                            u_max = (u + w) / tex_width
                            v_min = 1.0 - ((v + h) / tex_height)
                            
                            f.write(f"vt {u_min} {v_min}\n")
                            f.write(f"vt {u_max} {v_min}\n")
                            f.write(f"vt {u_max} {v_max}\n")
                            f.write(f"vt {u_min} {v_max}\n")
                        else:
                            # Face non définie dans le mapping UV -> (0,0)
                            f.write("vt 0.0 0.0\n")
                            f.write("vt 1.0 0.0\n")
                            f.write("vt 1.0 1.0\n")
                            f.write("vt 0.0 1.0\n")
                            
                    else:
                        # Fallback (0,0)
                        f.write("vt 0.0 0.0\n")
                        f.write("vt 1.0 0.0\n")
                        f.write("vt 1.0 1.0\n")
                        f.write("vt 0.0 1.0\n")

                # Générer les faces
                faces = generate_cube_faces(vertex_count, size)
                
                for face_idx, face in enumerate(faces):
                    uv_offset = uv_base + (face_idx * 4)
                    f.write(f"f {face[0]}/{uv_offset} {face[1]}/{uv_offset+1} {face[2]}/{uv_offset+2} {face[3]}/{uv_offset+3}\n")
                
                vertex_count += 8
                uv_count += len(faces) * 4
                uv_count += 24  # 6 faces * 4 UVs par face
                f.write("\n")
        
        # Ajouter les animations si présentes
        if animation_data:
            f.write("\n# Animations\n")
            animations = animation_data.get("animations", {})
            
            for anim_name, anim_data in animations.items():
                loop = 1 if anim_data.get("loop", False) else 0
                length = anim_data.get("animation_length", 1.0)
                
                f.write(f"anim {anim_name} {length} {loop}\n")
                
                # Traiter les keyframes de chaque bone
                bones_data = anim_data.get("bones", {})
                for bone_name, bone_anim in bones_data.items():
                    
                    # Rotation keyframes
                    if "rotation" in bone_anim:
                        rotations = bone_anim["rotation"]
                        for time_str, rotation in rotations.items():
                            time = float(time_str)
                            f.write(f"key {time} {bone_name} {rotation[0]} {rotation[1]} {rotation[2]}\n")
                
                f.write("\n")
    
    print(f"✓ Conversion réussie: {output_path}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python bedrock_to_obp.py <model.json> [animation.json] [output.obp]")
        print("\nExemples:")
        print("  python bedrock_to_obp.py models/anime_test.json")
        print("  python bedrock_to_obp.py models/anime_test.json animations/model.animation.json")
        print("  python bedrock_to_obp.py models/anime_test.json animations/model.animation.json output.obp")
        sys.exit(1)
    
    model_path = sys.argv[1]
    animation_path = sys.argv[2] if len(sys.argv) > 2 else None
    output_path = sys.argv[3] if len(sys.argv) > 3 else None
    
    convert_bedrock_to_obp(model_path, animation_path, output_path)


if __name__ == "__main__":
    main()
