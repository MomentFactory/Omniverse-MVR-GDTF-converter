import sys
import os

def main():
    os.environ["PATH"] = __file__ + os.pathsep + os.environ["PATH"]
    
    if len(sys.argv) <= 2:
        print("Need at least 2 arguments")
        return
    
    from pyassimp import load, export
    inputFile = sys.argv[1]
    outputFile = sys.argv[2]

    print("Input 3ds file:" + inputFile)
    print("output file: " + outputFile)

    with load(inputFile) as scene:
        export(scene, outputFile, "gltf2")


if __name__ == "__main__":
    main()

