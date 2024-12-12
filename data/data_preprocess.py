import os
import numpy as np
from PIL import Image, ImageOps
from skimage import measure     #install scikit-image
from plyfile import PlyData, PlyElement
import cv2
from scipy.ndimage import gaussian_filter

# process one scan
def load_single_jpeg(input_file):
    img = Image.open(input_file).convert("L")
    # print(f"{np.array(img).shape}") 
    img_resize = img.resize(img.size)
    # print(f"New Shape: {np.array(img_resize).shape}\n")
    return np.array(img_resize)

#downsize image
def downsize_single_jpeg(input_file, output_file, scale_factor = 0.2):
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
    newvolume = Form3Dvolume(volume)
    vertices, faces, normals, values = measure.marching_cubes(newvolume, level=iso_level)
    # print(f"Extracted {len(vertices)} vertices and {len(faces)} faces\n")
    return vertices, faces


def Form3Dvolume(volume):
    width, height = volume.shape
    zeros = np.zeros((width, height))
    return np.stack((volume, zeros), axis=-1)


def compute_normal(volume):
    dz_dx = np.gradient(volume, axis = 1)
    dz_dy = np.gradient(volume, axis = 0)
    normals = np.dstack((-dz_dx, -dz_dy, np.ones_like(volume)))
    #normals /= np.linalg.norm(normals, axis = 2)[:,:,np.newaxis]

    return normals


def VertexFacesGeneration(volume):
    width, height = volume.shape
    vertices = []
    faces = []
    normals = compute_normal(volume)
    
    for x in range(width):
        for y in range(height):
            vx,vy,vz = normals[x,y]
            vertices.append([x,y,0,vx,vy,vz,volume[x,y]]) # x,y,z,vx,vy,vz,s for learnply
        
    for x in range(width-1):
        for y in range(height-1):
            v0 = x*height + y
            v1 = v0 + 1
            v2 = v0 + height
            v3 = v2 + 1
            faces.append([v0,v1,v3,v2]) #ensure CCW

    return np.array(vertices), np.array(faces)


def export_to_ply(vertices, faces, output_file):
    #mesh = trimesh.Trimesh(vertices = vertices, faces = faces)
    #mesh.export(output_file, file_type='ply', encoding='ascii')     #ascii format to view 
    
    vertex_data = np.array([(v[0], v[1], v[2], v[3], v[4], v[5], v[6]) for v in vertices], 
                           dtype=[('x', 'f8'), ('y', 'f8'), ('z', 'f8'),('vx', 'f8'), ('vy', 'f8'), ('vz', 'f8'), ('s', 'f8')]) 
    
    face_data = np.array([(f,) for f in faces], dtype=[('vertex_indices', 'int32', (4,))])
    
    vertex_element = PlyElement.describe(vertex_data, 'vertex') 
    face_element = PlyElement.describe(face_data, 'face') 
    ply_data = PlyData([vertex_element, face_element], text=True) 
    ply_data.write(output_file) 
    
    # print(f"PLY file: {output_file}")

def process_ct_scan(input_file, downsize_file, output_file):
    #downsize image
    downsize_single_jpeg(input_file, downsize_file, scale_factor=0.1)

    #load and normalize the volume
    volume = load_single_jpeg(downsize_file)           
    #normalized_volume = normalize_volume(volume)

    #make scalar field injective
    epsilon = 1e-6
    volume_injective = volume + epsilon * np.random.random(volume.shape)

    #extract and convert to PLY
    #vertices, faces = extract_surface(volume_injective, iso_level=0.5)
    sigma = 5.0 # change this for different level of smoothing
    smoothed_volume = gaussian_filter(volume_injective,sigma)
    vertices, faces = VertexFacesGeneration(smoothed_volume)
    export_to_ply(vertices, faces, output_file)

if __name__ == "__main__":
    input_files = [
        "01.jpeg",
        "02.jpeg",
        "N01.jpeg",
        "N010.jpeg",
        "V01.jpeg",
        "V010.jpeg"
    ]

    downsize_dir = "downsized_ct_scans"
    os.makedirs(downsize_dir, exist_ok=True)

    #normalize scalar field
    for input_file in input_files:
        filename = os.path.basename(input_file).split('.')[0]
        downsize_file = os.path.join(downsize_dir, f"{filename}_downsize.jpeg")
        output_file = f"scalar_data/{filename}_scans.ply"
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


