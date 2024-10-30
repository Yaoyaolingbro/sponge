#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) {
        _EOF = true;
        _EOF_idx = index + data.size();
    }

    size_t idx = index;
    if (idx <= _next_assembled_idx) {
        if (idx + data.size() > _next_assembled_idx) {
            auto content = data.substr(_next_assembled_idx - idx);
            auto write_count = _output.write(content);
            _next_assembled_idx += write_count;

            // 合并现有缓存数据并继续写入
            auto iter = _unassembled.begin();
            while (iter != _unassembled.end() && iter->first <= _next_assembled_idx) {
                auto chip_size = iter->second.size();
                if (iter->first + chip_size >= _next_assembled_idx) {
                    auto existed_str = iter->second.substr(_next_assembled_idx - iter->first);
                    auto add_count = _output.write(existed_str);
                    _next_assembled_idx += add_count;
                }
                _unassembled_bytes -= chip_size;
                iter = _unassembled.erase(iter);
            }
        }
    } else {
        string s = data;
        auto iter = _unassembled.lower_bound(idx);

        // 后向合并
        while (iter != _unassembled.end() && iter->first <= idx + s.size()) {
            auto chip_size = iter->second.size();
            if (iter->first + chip_size > idx + s.size()) {
                s += iter->second.substr(idx + s.size() - iter->first);
            }
            _unassembled_bytes -= chip_size;
            iter = _unassembled.erase(iter);
        }

        // 前向合并
        iter = _unassembled.lower_bound(idx);
        if (iter != _unassembled.begin() && !_unassembled.empty()) {
            iter--;
            auto chip_size = iter->second.size();
            if (iter->first + chip_size >= idx) {
                if (iter->first + chip_size > idx + s.size()) {
                    s = iter->second;
                } else {
                    s = iter->second + s.substr(iter->first + chip_size - idx);
                }
                _unassembled_bytes -= chip_size;
                idx = iter->first;
                _unassembled.erase(iter);
            }
        }

        auto store_count = std::min(s.size(), _capacity - _output.buffer_size() - _unassembled_bytes);
        _unassembled_bytes += store_count;
        _unassembled.emplace(idx, s.substr(0, store_count));
    }

    if (_EOF && _next_assembled_idx >= _EOF_idx) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
