#pragma once

/*
 * Based on "The FFX Mode of Operation for Format-Preserving Encryption"
 * by Bellare, Rogaway, and Spies.
 * http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/ffx/ffx-spec.pdf
 *
 * This implementation follows FFX-A2, and is hard-coded to use:
 *   radix:    2 (binary)
 *   addition: 0 (character-wise addition, i.e., XOR)
 *   method:   2 (alternating Feistel)
 *   F:        CBC-MAC using AES
 */

#include <crypto/cbcmac.hh>

class ffx {
 public:
    ffx(AES *key) {
        k = key;
    }

    void encrypt(const uint8_t *pt, uint8_t *ct, uint nbits,
                 const std::vector<uint8_t> &t) const
    {
        const ffx_mac_header h(nbits, t);

        uint64_t a, b, c;
        mem_to_u64(pt, &a, &b, h.s, h.n - h.s);

        for (int i = 0; i < h.rounds; i++) {
            c = a ^ f(h, i, b, t);
            a = b;
            b = c;
        }

        u64_to_mem(a, b, h.s, h.n - h.s, ct);
    }

    void decrypt(const uint8_t *ct, uint8_t *pt, uint nbits,
                 const std::vector<uint8_t> &t) const
    {
        const ffx_mac_header h(nbits, t);

        uint64_t a, b, c;
        mem_to_u64(ct, &a, &b, h.s, h.n - h.s);

        for (int i = h.rounds - 1; i >= 0; i--) {
            c = b;
            b = a;
            a = c ^ f(h, i, b, t);
        }

        u64_to_mem(a, b, h.s, h.n - h.s, pt);
    }

 private:
    struct ffx_mac_header {
        uint16_t ver;
        uint8_t method;
        uint8_t addition;
        uint8_t radix;
        uint8_t n;
        uint8_t s;
        uint8_t rounds;
        uint64_t tlen;

        ffx_mac_header(uint64_t narg, const std::vector<uint8_t> &t)
            : ver(1), method(2), addition(0), radix(2), n(narg),
              s(split(n)), rounds(rnds(n)), tlen(t.size())
            {
                assert(n <= 128);
            }
    };

    static uint64_t split(uint64_t n) {
        return n / 2;
    }

    static uint8_t rnds(uint64_t n) {
        assert(n >= 8 && n <= 128);

        if (n <= 9)
            return 36;
        if (n <= 13)
            return 30;
        if (n <= 19)
            return 24;
        if (n <= 31)
            return 18;
        return 12;
    }

    /*
     * For non-multiple-of-8-bit values, the bits come from MSB.
     */
    static void mem_to_u64(const uint8_t *p,
                           uint64_t *a, uint64_t *b,
                           uint abits, uint bbits)
    {
        assert(abits <= 64 && bbits <= 64);

        *a = 0;
        *b = 0;

        while (abits >= 8) {
            *a = *a << 8 | *p;
            p++;
            abits -= 8;
        }

        if (abits) {
            *a = *a << abits | *p >> (8 - abits);
            uint8_t pleft = *p & ((1 << (8 - abits)) - 1);
            if (bbits < 8 - abits) {
                *b = pleft >> (8 - bbits);
                bbits = 0;
            } else {
                *b = pleft;
                bbits -= 8 - abits;
            }
            p++;
        }

        while (bbits >= 8) {
            *b = *b << 8 | *p;
            p++;
            bbits -= 8;
        }

        if (bbits)
            *b = *b << bbits | *p >> (8 - bbits);
    }

    static void u64_to_mem(uint64_t a, uint64_t b,
                           uint64_t abits, uint64_t bbits,
                           uint8_t *p)
    {
        assert(abits <= 64 && bbits <= 64);

        while (abits >= 8) {
            *p = a >> (abits - 8);
            p++;
            abits -= 8;
        }

        if (abits) {
            *p = a & ((1 << abits) - 1);
            if (bbits < 8 - abits) {
                *p = (*p << bbits | (b & ((1 << bbits) - 1))) << (8 - abits - bbits);
                bbits = 0;
            } else {
                *p = *p << (8 - abits) | b >> (bbits - (8 - abits));
                bbits -= (8 - abits);
            }
            p++;
        }

        while (bbits >= 8) {
            *p = b >> (bbits - 8);
            p++;
            bbits -= 8;
        }

        if (bbits)
            *p = b << (8 - bbits);
    }

    uint64_t f(const ffx_mac_header &h, uint8_t i, uint64_t b,
               const std::vector<uint8_t> &t) const
    {
        cbcmac<AES> mac(k);
        mac.update(&h, sizeof(h));
        mac.update(&t[0], t.size());

        struct {
            uint8_t zero[16 + 7];
            uint8_t i;
            uint64_t b;
        } tail;

        uint tailoff = t.size() % 16;
        if (tailoff + 16 <= sizeof(tail.zero))
            tailoff += 16;

        memset(tail.zero, 0, sizeof(tail.zero));
        tail.i = i;
        tail.b = b;
        mac.update(&tail.zero[tailoff], sizeof(tail) - tailoff);

        uint8_t out[16];
        mac.final(out);

        uint m = (i % 2) ? (h.n -  h.s) : h.s;
        uint64_t r, dummy;
        mem_to_u64(out, &r, &dummy, m, 0);
        return r;
    }

    AES *k;
};

template<uint nbits>
class ffx_block_cipher {
 public:
    ffx_block_cipher(const ffx *farg, const std::vector<uint8_t> &targ)
        : f(farg), t(targ) {}

    void block_encrypt(const uint8_t *ptext, uint8_t *ctext) const {
        f->encrypt(ptext, ctext, nbits, t);
    }

    void block_decrypt(const uint8_t *ctext, uint8_t *ptext) const {
        f->decrypt(ctext, ptext, nbits, t);
    }

    static const size_t blocksize = (nbits + 7) / 8;

 private:
    const ffx *f;
    const std::vector<uint8_t> &t;
};