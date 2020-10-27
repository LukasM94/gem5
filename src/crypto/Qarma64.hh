/*
MIT License

Copyright (c) 2019 Phantom1003

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "stdlib.h"

typedef unsigned long long int const_t;
typedef unsigned long long int tweak_t;
typedef unsigned long long int text_t;
// typedef unsigned long long int key_t;
typedef unsigned char          cell_t;

namespace Crypto
{
  class Qarma64
  {
    public:
      static Qarma64* instance();
      ~Qarma64();
      void setTweak(tweak_t tweak);
      void setKeyK(key_t k0);
      void setKeyW(key_t w0);
      text_t enc(text_t plaintext, int rounds = ROUNDS);
      text_t dec(text_t ciphertext, int rounds = ROUNDS);
    private:
      Qarma64();
      static Qarma64* instance_;
      tweak_t tweak_;
      key_t   w0_;
      key_t   k0_;
      const static int ROUNDS = 5;
  };
}
