#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <utility>

using u64 = unsigned long long;

struct ByteWriter {
  char* data {};
  u64 size {};
  u64 capacity {};

  ByteWriter(ByteWriter const&) = delete;
  ~ByteWriter() { free(std::exchange(data, nullptr)); }

  void put(char c) {
    reserve(size + 1);
    data[size++] = c;
  }

  void put(char const* bytes, u64 count) {
    reserve(size + count);
    std::memcpy(&data[size], bytes, count);
    size += count;
  }

  void reserve(u64 at_least) {
    if (capacity >= at_least)
      return;
    if (at_least && !capacity)
      capacity = 1;
    while (capacity < at_least)
      capacity *= 2;
    data = reinterpret_cast<char*>(realloc(data, capacity));
  }
};

struct State {
  ByteWriter out;
  
  bool in_string = false;

  void on(char const* c, u64 size) {
    auto i = 0ull;
    auto start = 0ull;

    if (in_string)
      goto inside_string;

    out_of_string:
    while (i < size) {
      if (c[i] == '"') {
        i++;
        in_string = true;
        goto inside_string;
      } else if (c[i] == ',') {
        if (i > start)
          out.put(&c[start], i - start);
        out.put('\t');
        ++i;
        start = i;
      } else {
        ++i;
      }
    }

    inside_string:
    while (i < size) {
      if (c[i] == '"') {
        i++;
        in_string = false;
        goto out_of_string;
      } else {
        ++i;
      }
    }

    if (i > start)
      out.put(&c[start], i - start);
  }
};

int main() {
  State s {};
  char data[4096 * 256];
  while (true) {
    auto n = read(STDIN_FILENO, data, sizeof(data));
    if (!n)
      break;
    s.on(data, n);
  }

  auto out = s.out.data;
  auto end = out + s.out.size;
  while (out < end) {
    auto n = write(STDOUT_FILENO, out, end - out);
    if (n < 0)
      return 1;
    out += n;
  }
}
