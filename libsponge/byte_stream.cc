#include "byte_stream.hh"
#include <iostream>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity)
    : _buffer(""), _capacity(capacity), _bytes_written(0), _bytes_read(0), _input_ended(false), _error(false) {}

size_t ByteStream::write(const string &data) {
    if (_input_ended) {
        return 0;
    }

    size_t len_ = min(data.size(), _capacity - _buffer.size());
    _bytes_written += len_;
    _buffer += data.substr(0, len_);
    return len_;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t len_ = min(len, _buffer.size());
    return _buffer.substr(0, len_);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t len_ = min(len, _buffer.size());
    _buffer = _buffer.substr(len_);
    _bytes_read += len_;
    return;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ret = peek_output(len);
    pop_output(len);
    return ret;
}

void ByteStream::end_input() { _input_ended = true; }

bool ByteStream::input_ended() const { return _input_ended; }

size_t ByteStream::buffer_size() const { return _buffer.size(); }

bool ByteStream::buffer_empty() const { return _buffer.empty(); }

bool ByteStream::eof() const { return buffer_empty() && input_ended(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - _buffer.size(); }
