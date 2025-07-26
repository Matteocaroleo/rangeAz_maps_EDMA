def generate_test_hex_file(filename="test_input.txt", total_bytes=8192):
    with open(filename, "w") as f:
        for i in range(total_bytes):
            byte_str = f"{i % 256:02X}"  # Wrap dopo FF
            f.write(byte_str)
            if (i + 1) % 16 == 0:
                f.write('\n')  # 16 byte = 128 bit → newline
            else:
                f.write(' ')
    print(f"✅ File di test generato: {filename} ({total_bytes} byte)")

if __name__ == "__main__":
    generate_test_hex_file()
