/*
    Test task for Kaspersky, SPD-team, by Andrei Nekrasov.
    Implemented for GNU/Linux
    This program implements a simple static site generator, by recursively
    copying input directory to output directory, and
    transforming every "*.gmi" file to "*.html" in the process.

    It's assumed, that the output directory does not exist and that we have
    rights to create it and write into it.
    Uses C++ 17 standard.

    Build:
        g++ -std=c++17 -o <name> main.cpp
    Run:
        ./<name> <input_directory> <output_directory>
*/

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Tags in ".gmi" file
enum TagType {
    PlainText,
    FirstHeader,
    SecondHeader,
    ThirdHeader,
    ListElement,
    Quote,
    Link,
    PreformattedText
};

bool isTagMatching(std::string line, std::string tag) {
    return line.size() >= tag.size() && line.substr(0, tag.size()) == tag;
}

TagType determineLineTag(std::string line) {
    if (isTagMatching(line, "# ")) {
        return FirstHeader;
    } else if (isTagMatching(line, "## ")) {
        return SecondHeader;
    } else if(isTagMatching(line, "### ")) {
        return ThirdHeader;
    } else if(isTagMatching(line, "* ")) {
        return ListElement;        
    } else if(isTagMatching(line, ">")) {
        return Quote;
    } else if(isTagMatching(line, "=> ")) {
        return Link;
    } else if(isTagMatching(line, "```")) {
        return PreformattedText;
    }
    return PlainText;
}

void processModes(std::ostringstream &oss, 
        TagType mode, std::string line) {
    /*
        writes to oss gem-tags transformed to corresponding html
        (except for <pre>, witch writes only first half of the tag)
        magic numbers in line.erase(0, <x>) stand 
        for length of gem-tag (including space)
    */
    if (mode == PreformattedText) {
        oss << "<pre>" << line.erase(0, 4);
    } else if (mode == PlainText) {
        oss << line << "\n";
    } else if (mode == FirstHeader) {
        oss << "<h1>" << line.erase(0, 2) << "</h1>\n";
    } else if (mode == SecondHeader) {
        oss << "<h2>" << line.erase(0, 3) << "</h2>\n";
    } else if (mode == ThirdHeader) {
        oss << "<h3>" << line.erase(0, 4) << "</h3>\n";
    } else if (mode == ListElement) {
        oss << "<li>" << line.erase(0, 2) << "</li>\n";
    } else if (mode == Quote) {
        oss << "<blockquote>" << line.erase(0, 2) << "</blockquote>\n";
    } else if (mode == Link) {
        // assumes that a link can't contain spaces
        size_t pos = line.erase(0, 3).find(' ');
        if (pos == std::string::npos) {
            // write the whole thing as plain text, it's not a valid link
            oss << line << "\n";
            return;
        }
        std::string href = line.substr(0, pos);
        std::string name = line.erase(0, pos + 1); // 1 is for space
        oss << "<a href=\"" << href << "\">" << name << "</a>\n"; 
    }
}

std::string processGemText(std::string text) {
    /*
        converts gem-text to html-text
    */
    std::istringstream iss(text);
    std::ostringstream oss;
    TagType mode = PlainText; // dummy starting value
    for (std::string line; std::getline(iss,line); ) { // read text by line
        if (mode != PreformattedText) {
            mode = determineLineTag(line);
            processModes(oss, mode, line);
        } else {
            if (determineLineTag(line) == PreformattedText) {
                oss << "</pre>";
                mode = PlainText; // dummy value after "pre"-text
                continue;
            }
            oss << line << "\n"; // write without processing tags
        }
    }
    return oss.str();
}

void rewriteGemFile(fs::path p) {
    /*
        replaces "*.gmi" file by path p with "*.html" and performs 
        tag-processing
    */
    fs::path oldPath = p;
    p.replace_extension(".html");
    // read text from ".gmi" file
    std::ifstream inputFile(oldPath);
    if (!inputFile) {
        std::cerr << "couldn't read " << oldPath << "\n";
        return;
    }
    std::stringstream buf;
    buf << inputFile.rdbuf();
    std::string text = buf.str();
    // convert gem-text to html-format
    std::string reformattedText = processGemText(text);

    // create new html-file and write reformatted text
    std::ofstream outputFile(p);
    outputFile << reformattedText;
    fs::remove(oldPath); // remove old gem file

    inputFile.close();
    outputFile.close();
}

void copyRDir(std::string inPath, std::string outPath) {
    /*
        recursively copies directory inPath to directory outPath
        can throw
    */
    fs::copy(inPath, outPath, fs::copy_options::recursive);
    std::vector<fs::path> gemFiles;
    for (const auto &dirEntry : fs::recursive_directory_iterator(outPath)) {
        if (dirEntry.path().extension() == ".gmi") {
            gemFiles.push_back(dirEntry.path());
        }
    }
    for (auto &p : gemFiles) {
        rewriteGemFile(p);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cout << "Usage: <exec_name> <input_folder> <output_folder\n"
            "Note that output folder must not exist\n"; 
        return 1;
    }
    try {
        copyRDir(argv[1], argv[2]);
    } catch (fs::filesystem_error const& e) {
        std::cerr << e.what() << "\nFailed to copy directory\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred\n";
        return 1;
    }
    return 0;
}