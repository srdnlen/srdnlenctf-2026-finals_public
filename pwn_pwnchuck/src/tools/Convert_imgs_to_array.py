from PIL import Image
fname = input("Insert the file name: ")

name = fname.split(".")[0]

img = Image.open(fname).convert("P", palette=Image.ADAPTIVE,colors=255)
width, height = img.size
palette = img.getpalette()
img_data = list(img.getdata())

n_used = max(img_data) + 1
colors = [tuple(palette[i:i+3]) for i in range(0, n_used * 3, 3)]

print("Starting....")

out = f"static const char *const {name}_rows [{height}] =" +'{\n\t'
for i in range(height):
    out +="\""
    for j in range(width):
        out +=f"\\x{img_data[i * width + j]:02x}" 
        r,g,b = colors[img_data[i * width + j]]
    out += "\",\n\t"
out += '};'


cs = [f"\t{{{r},{g},{b}}}" for r, g, b in colors]

pal = f"const u8 pal_{name}[{len(cs)}][3] = ""{ \n" + ",\n".join(cs)+ " \n};"

print("DONE! WRITING TO FILE....")

with open('./out_images/'+name+".c","w") as file:
    file.write(f"/*-----{name} IMAGE DATA -----*/\n")
    file.write(out)
    file.write("\n\n")
    file.write(f"/*-----{name} PALETTE -----*/")
    file.write("\n\n")
    file.write(pal)
    file.write("\n\n")
    file.write(f"const Sprite art_{name} =" +'{' +f"{name}_rows, {width}, {height}" + '};')
    file.write("\n\n")
    file.write(f"extern const u8 pal_{name}[{len(cs)}][3];")
