def generate_test_hex_input_1024x64(filename="test_input_1024x64.txt"):
    total_int16 = 1024 * 64               # 65536 valori
    total_bytes = total_int16 * 2         # 131072 byte

    with open(filename, "w") as f:
        for i in range(total_bytes):
            byte_val = i % 256  # sequenza ciclica 00-FF
            f.write(f"{byte_val:02X}")
            if (i + 1) % 16 == 0:
                f.write("\n")  # newline ogni 16 byte (128 bit)
            else:
                f.write(" ")
    print(f"✅ File generato: {filename} (Matrice 1024x64 int16 → {total_bytes} byte)")

if __name__ == "__main__":
    generate_test_hex_input_1024x64()
