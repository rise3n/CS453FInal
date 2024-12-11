import os
import numpy as np
from PIL import Image, ImageOps
from skimage import measure     #install scikit-image
import trimesh                  #install trimesh
import cv2

# process one scan
def load_single_jpeg(input_file):
    size = (4200, 3480)
    img = Image.open(input_file).convert("L")
    # print(f"{np.array(img).shape}") 
    img_resize = img.resize(size)
    # print(f"New Shape: {np.array(img_resize).shape}\n")
    return np.array(img_resize)

#downsize image
def downsize_single_jpeg(input_file, output_file, scale_factor = 0.5):
    image = cv2.imread(input_file)
    if image is None:
        print(f"Unable to read the image at {input_file}")
        return
    
    new_width = int(image.shape[1] * scale_factor)
    new_height = int(image.shape[0] * scale_factor)
    downsampled_image = cv2.resize(image, (new_width, new_height))

    cv2.imwrite(output_file, downsampled_image)
    # print(f"Processed and saved: {output_file}")
    return output_file

def normalize_volume(volume):
    return (volume - np.min(volume)) / (np.max(volume) - np.min(volume))

def extract_surface(volume, iso_level = 0.5):
    vertices, faces, normals, values = measure.marching_cubes(volume, level=iso_level)
    # print(f"Extracted {len(vertices)} vertices and {len(faces)} faces\n")
    return vertices, faces    

def export_to_ply(vertices, faces, output_file):
    mesh = trimesh.Trimesh(vertices = vertices, faces = faces)
    mesh.export(output_file, file_type='ply', encoding='ascii')     #ascii format to view 
    # print(f"PLY file: {output_file}")

def process_ct_scan(input_file, downsize_file, output_file):
    #downsize image
    downsize_single_jpeg(input_file, downsize_file)

    #load and normalize the volume
    volume = load_single_jpeg(downsize_file)           
    normalized_volume = normalize_volume(volume)

    #make scalar field injective
    epsilon = 1e-6
    volume_injective = normalized_volume + epsilon * np.random.random(normalized_volume.shape)

    #extract and convert to PLY
    vertices, faces = extract_surface(volume_injective, iso_level=0.5)
    export_to_ply(vertices, faces, output_file)

if __name__ == "__main__":
    input_files = [
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\01.jpeg",
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\02.jpeg",
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\N01.jpeg",
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\N010.jpeg",
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\V01.jpeg",
        r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\V010.jpeg"
    ]

    downsize_dir = r"C:\Users\joyli\junior_fall\sci_visual\CS453FInal\data\downsized_ct_scans"
    os.makedirs(downsize_dir, exist_ok=True)

    #normalize scalar field
    for input_file in input_files:
        filename = os.path.basename(input_file).split('.')[0]
        downsize_file = os.path.join(downsize_dir, f"{filename}_downsize.jpeg")
        output_file = f"{filename}_scans.ply"
        process_ct_scan(input_file, downsize_file, output_file)
        print(f"Processed: {input_file}")

    # volume = load_single_jpeg(downsize_file)           
    # normalized_volume = normalize_volume(volume)  
    # #make scalar field injective  
    # epsilon = 1e-6
    # volume_injective = normalized_volume + epsilon * np.random.random(normalized_volume.shape)
    # #extraction and conversion
    # vertices, faces = extract_surface(normalized_volume, iso_level = 0.5)
    # export_to_ply(vertices, faces, output_file)


