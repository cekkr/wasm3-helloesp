import os
import sys
from pathlib import Path

def find_c_files(base_path, relative_to=None):
    """Find all .c files recursively and return their paths relative to relative_to"""
    if relative_to is None:
        relative_to = base_path
    
    c_files = []
    base = Path(base_path)
    rel_base = Path(relative_to)
    
    for root, _, files in os.walk(base):
        for file in files:
            if file.endswith('.c'):
                abs_path = Path(root) / file
                try:
                    rel_path = abs_path.relative_to(rel_base)
                    c_files.append(str(rel_path))
                except ValueError:
                    continue
    
    return sorted(c_files)

def find_include_dirs(base_path, relative_to=None):
    """Find all directories containing .h files"""
    if relative_to is None:
        relative_to = base_path
        
    include_dirs = set()
    base = Path(base_path)
    rel_base = Path(relative_to)
    
    for root, _, files in os.walk(base):
        if any(f.endswith('.h') for f in files):
            try:
                rel_path = Path(root).relative_to(rel_base)
                include_dirs.add(str(rel_path))
            except ValueError:
                continue
    
    return sorted(include_dirs)

def generate_cmake_content(wasm3_path):
    # Find source files
    src_files = find_c_files(wasm3_path)
    
    # Find include directories
    include_dirs = find_include_dirs(wasm3_path)
    
    # Generate CMake content
    cmake_content = ['idf_component_register(']
    
    # Add SRCS
    cmake_content.append('    SRCS')
    for src in src_files:
        cmake_content.append(f'        "{src}"')
    
    # Add INCLUDE_DIRS
    cmake_content.append('    INCLUDE_DIRS')
    for inc_dir in include_dirs:
        cmake_content.append(f'        "{inc_dir}"')
    
    # Add basic requirements
    cmake_content.append('    REQUIRES')
    cmake_content.append('        "nvs_flash"')
    
    cmake_content.append(')')
    
    return '\n'.join(cmake_content)

# Well, I created it, but I think it's useless
def main():
    wasm3_path = "./main/wasm3"
    if len(sys.argv) >= 2:
        wasm3_path = sys.argv[1]
    
    if not os.path.exists(wasm3_path):
        print(f"Error: Path {wasm3_path} does not exist")
        sys.exit(1)
    
    cmake_content = generate_cmake_content(wasm3_path)
    
    # Write to CMakeLists.txt
    output_path = os.path.join(os.path.dirname(wasm3_path), "CMakeLists.txt")
    with open(output_path, 'w') as f:
        f.write(cmake_content)
    
    print(f"Generated CMakeLists.txt at {output_path}")
    print("\nContent:")
    print(cmake_content)

if __name__ == "__main__":
    main()