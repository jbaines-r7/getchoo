#include <cstdlib>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace
{
    void banner()
    {
        std::cout << std::endl;
        std::cout << "  MM'\"\"\"\"\"`MM            dP            dP" << std::endl;   
        std::cout << "  M' .mmm. `M            88            88" << std::endl;   
        std::cout << "  M  MMMMMMMM .d8888b. d8888P .d8888b. 88d888b. .d8888b. .d8888b." << std::endl;   
        std::cout << "  M  MMM   `M 88ooood8   88   88'  `\"\" 88'  `88 88'  `88 88'  `88" << std::endl;
        std::cout << "  M. `MMM' .M 88.  ...   88   88.  ... 88    88 88.  .88 88.  .88" << std::endl;
        std::cout << "  MM.     .MM `88888P'   dP   `88888P' dP    dP `88888P' `88888P'" << std::endl;   
        std::cout << "  MMMMMMMMMMM" << std::endl;
        std::cout << "                           jbaines-r7 🦞" << std::endl;
        std::cout << std::endl;
    }
    std::string bytes2hex(const std::string& p_input)
    {
        std::stringstream ss;

        ss << std::hex << std::uppercase << std::setfill( '0' );
        std::for_each(p_input.cbegin(), p_input.cend(), [&](unsigned int c) { ss << std::setw( 2 ) << (c & 0xff); } );

        return ss.str();
    }
}

int main(int p_argc, char** p_argv)
{
    banner();

    if (p_argc != 2)
    {
        std::cerr << "[-] Please provide an sgz file." << std::endl;
        return EXIT_FAILURE;
    }

    std::ifstream inputFile(p_argv[1], std::ifstream::in | std::ifstream::binary);
    if (!inputFile.is_open() || !inputFile.good())
    {
        std::cerr << "[-] Failed to open the provided file: " << p_argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    std::string input((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    std::cout << "[+] File read. Size: " << input.size() << std::endl;
    if (input.size() < 17)
    {
        std::cout << "[-] The file is too small to be an sgz." << std::endl;
        return EXIT_FAILURE;
    }

    std::string fingerprint(input.begin(), input.begin() + 16);
    std::cout << "[+] Fingerprint: " << bytes2hex(fingerprint) << std::endl;

    input.erase(0, 16);
    input.erase(0, 1); // no idea why we skip this or wtf it is

    std::cout << "[+] Unpacking to out.lzma" << std::endl;
    while (input.empty() == false)
    {
        // length, type
        std::uint32_t length = *reinterpret_cast<uint32_t*>(&input[0]);
        std::uint8_t type = input[4];
        input.erase(0, 5);

        if (length == 0xffffffff && type == 0)
        {
            std::cout << "[+] End of lzma file extraction" << std::endl;
            input.clear();
        }
        else if (type == 3)
        {
            std::string lzma(input.data(), input.data() + length);
            std::ofstream out("out.lzma");
            out << lzma;
            out.close();
        }

        input.erase(0, length);
    }

    std::cout << "[+] Decompressing to out" << std::endl;
    system("lzma -d out.lzma");

    std::cout << "[+] Loading the decompressed file" << std::endl;
    std::ifstream decompressed("out", std::ifstream::in | std::ifstream::binary);
    if (!decompressed.is_open() || !decompressed.good())
    {
        std::cerr << "[!] Failed to open the lzma -d created file: out" << std::endl;
        return EXIT_FAILURE;
    }

    input.assign((std::istreambuf_iterator<char>(decompressed)), std::istreambuf_iterator<char>());
    decompressed.close();

    std::cout << "[+] Creating ./tmp/ to write files into" << std::endl;
    system("rm -rf ./tmp/; mkdir ./tmp/");

    while (input.empty() == false)
    {
        std::uint16_t str_length = *reinterpret_cast<uint16_t*>(&input[0]);
        std::string filename(input.data() + 2, input.data() + str_length + 2);
        input.erase(0, 2);
        input.erase(0, str_length);

        // looool directory traversal
        std::cout << "[+] Creating tmp/" << filename << std::endl;
        std::ofstream out("tmp/" + filename, std::ifstream::trunc | std::ifstream::binary);

        // note that this is wildly inefficient
        std::uint32_t file_length = *reinterpret_cast<uint32_t*>(&input[0]);
        input.erase(0, 4);
        out.write(input.data(), file_length);
        input.erase(0, file_length);
        out.close();
    }

    std::cout << "[+] Deleting ./out" << std::endl;
    system("rm ./out");
    std::cout << "[+] Done!" << std::endl;
    return EXIT_SUCCESS;
}