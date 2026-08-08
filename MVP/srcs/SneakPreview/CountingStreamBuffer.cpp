#include <fstream>
#include <iostream>
#include <streambuf>

class CountingStreamBuffer : public std::streambuf {

public:

    explicit CountingStreamBuffer(std::streambuf* sb)
        : sb_(sb), last_write_bytes_(0) {}

    int overflow(int c) {
        if (c != EOF) {
            ++last_write_bytes_;
            return sb_->sputc(c);
        }
        return EOF;
    }

    std::streamsize xsputn(const char* s, std::streamsize n) {
        last_write_bytes_ += n;
        return sb_->sputn(s, n);
    }

    int sync() {
        return sb_->pubsync();
    }

    std::streamsize getLastWriteBytes() const {
        return last_write_bytes_;
    }

    void resetLastWriteBytes() {
        last_write_bytes_ = 0;
    }

private:

    std::streambuf* sb_;
    std::streamsize last_write_bytes_;

};

int main() {

    std::ofstream out("output.txt");
    CountingStreamBuffer counting_buf(out.rdbuf());
    std::ostream counting_stream(&counting_buf);

    counting_stream << "Hello, ";
    std::cout << "Bytes written: " << counting_buf.getLastWriteBytes() << std::endl;
    counting_buf.resetLastWriteBytes();

    counting_stream << "world!";
    std::cout << "Bytes written: " << counting_buf.getLastWriteBytes() << std::endl;

    out.close();
    return 0;

}
