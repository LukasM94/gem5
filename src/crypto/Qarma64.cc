#include "crypt/Qarma64.hh"

// #include <assert.h>

// #include "base/trace.hh"
// #include "debug/QARMA64.hh"

Crypto::Qarma64* Crypto::Qarma64::instance_ = 0;

Crypto::Qarma64::Qarma64() :
  tweak_(-1),
  w0_(-1),
  k0_(-1)
{
  DPRINTF(QARMA64, "Qarma64::ctor\n");
}

Crypto::Qarma64::~Qarma64()
{
  DPRINTF(QARMA64, "Qarma64::dtor\n");
}

Crypto::Qarma64* Crypto::Qarma64::instance()
{
  if (instance_ == 0)
  {
    instance_ = new Qarma64();
  }
  return instance_;
}

void Crypto::Qarma64::setTweak(tweak_t tweak)
{
  DPRINTF(QARMA64, "Qarma64::setTweak <%lu>\n", tweak);
  tweak_ = tweak;
}

void Crypto::Qarma64::setKeyK(key_t k0)
{
  DPRINTF(QARMA64, "Qarma64::setKeyK <%lu>\n", k0);
  k0_ = k0;
}

void Crypto::Qarma64::setKeyW(key_t w0)
{
  DPRINTF(QARMA64, "Qarma64::setKeyW <%lu>\n", w0);
  w0_ = w0;
}

text_t Crypto::Qarma64::enc(text_t plaintext, int rounds)
{
  assert(tweak_ != -1 && w0_ != -1 && k0_ != -1);
  text_t ciphertext = qarma64_enc(plaintext, tweak_, w0_, k0_, rounds);
  DPRINTF(QARMA64, "Qarma64::enc <%lu> -> <%lu>\n", plaintext, ciphertext);
  return ciphertext;
}

text_t Crypto::Qarma64::dec(text_t ciphertext, int rounds)
{
  assert(tweak_ != -1 && w0_ != -1 && k0_ != -1);
  text_t plaintext =  qarma64_dec(ciphertext, tweak_, w0_, k0_, rounds);
  DPRINTF(QARMA64, "Qarma64::dec <%lu> -> <%lu>\n", ciphertext, plaintext);
  return plaintext;
}

