// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com
// Original code is licensed as follows:
/*
 * Copyright 2006 Jeremias Maerki in part, and ZXing Authors in part
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fxbarcode/pdf417/BC_PDF417HighLevelEncoder.h"

#include "fxbarcode/BC_UtilCodingConvert.h"
#include "fxbarcode/pdf417/BC_PDF417Compaction.h"
#include "fxbarcode/utils.h"
#include "third_party/bigint/BigIntegerLibrary.hh"

#define SUBMODE_ALPHA 0
#define SUBMODE_LOWER 1
#define SUBMODE_MIXED 2

namespace {

constexpr int32_t kLatchToText = 900;
constexpr int32_t kLatchToBytePadded = 901;
constexpr int32_t kLatchToNumeric = 902;
constexpr int32_t kShiftToByte = 913;
constexpr int32_t kLatchToByte = 924;

constexpr uint8_t kTextMixedRaw[] = {48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                     38, 13, 9,  44, 58, 35, 45, 46, 36, 47,
                                     43, 37, 42, 61, 94, 0,  32, 0,  0,  0};
constexpr uint8_t kTextPunctuationRaw[] = {
    59, 60, 62, 64, 91, 92, 93,  95, 96, 126, 33, 13,  9,   44, 58,
    10, 45, 46, 36, 47, 34, 124, 42, 40, 41,  63, 123, 125, 39, 0};

int32_t g_mixed[128] = {0};
int32_t g_punctuation[128] = {0};

}  // namespace

void CBC_PDF417HighLevelEncoder::Initialize() {
  Inverse();
}

void CBC_PDF417HighLevelEncoder::Finalize() {}

WideString CBC_PDF417HighLevelEncoder::EncodeHighLevel(WideString wideMsg,
                                                       Compaction compaction,
                                                       int32_t& e) {
  ByteString bytes;
  CBC_UtilCodingConvert::UnicodeToUTF8(wideMsg, bytes);
  size_t len = bytes.GetLength();
  WideString msg;
  msg.Reserve(len);
  for (size_t i = 0; i < len; i++) {
    wchar_t ch = bytes[i] & 0xff;
    if (ch == '?' && bytes[i] != '?') {
      e = BCExceptionCharactersOutsideISO88591Encoding;
      return WideString();
    }
    msg += ch;
  }
  std::vector<uint8_t> byteArr(bytes.begin(), bytes.end());
  len = msg.GetLength();
  WideString sb;
  sb.Reserve(len);
  size_t p = 0;
  int32_t textSubMode = SUBMODE_ALPHA;
  if (compaction == TEXT) {
    EncodeText(msg, p, len, sb, textSubMode);
  } else if (compaction == BYTES) {
    EncodeBinary(&byteArr, p, byteArr.size(), BYTE_COMPACTION, sb);
  } else if (compaction == NUMERIC) {
    sb += kLatchToNumeric;
    EncodeNumeric(msg, p, len, sb);
  } else {
    int32_t encodingMode = kLatchToText;
    while (p < len) {
      size_t n = DetermineConsecutiveDigitCount(msg, p);
      if (n >= 13) {
        sb += kLatchToNumeric;
        encodingMode = NUMERIC_COMPACTION;
        textSubMode = SUBMODE_ALPHA;
        EncodeNumeric(msg, p, n, sb);
        p += n;
      } else {
        size_t t = DetermineConsecutiveTextCount(msg, p);
        if (t >= 5 || n == len) {
          if (encodingMode != TEXT_COMPACTION) {
            sb += kLatchToText;
            encodingMode = TEXT_COMPACTION;
            textSubMode = SUBMODE_ALPHA;
          }
          textSubMode = EncodeText(msg, p, t, sb, textSubMode);
          p += t;
        } else {
          Optional<size_t> b =
              DetermineConsecutiveBinaryCount(msg, &byteArr, p);
          if (!b) {
            e = BCExceptionNonEncodableCharacterDetected;
            return L" ";
          }
          size_t b_value = b.value();
          if (b_value == 0) {
            b_value = 1;
          }
          if (b_value == 1 && encodingMode == TEXT_COMPACTION) {
            EncodeBinary(&byteArr, p, 1, TEXT_COMPACTION, sb);
          } else {
            EncodeBinary(&byteArr, p, b_value, encodingMode, sb);
            encodingMode = BYTE_COMPACTION;
            textSubMode = SUBMODE_ALPHA;
          }
          p += b_value;
        }
      }
    }
  }
  return sb;
}

void CBC_PDF417HighLevelEncoder::Inverse() {
  for (size_t l = 0; l < FX_ArraySize(g_mixed); ++l)
    g_mixed[l] = -1;

  for (uint8_t i = 0; i < FX_ArraySize(kTextMixedRaw); ++i) {
    uint8_t b = kTextMixedRaw[i];
    if (b != 0)
      g_mixed[b] = i;
  }

  for (size_t l = 0; l < FX_ArraySize(g_punctuation); ++l)
    g_punctuation[l] = -1;

  for (uint8_t i = 0; i < FX_ArraySize(kTextPunctuationRaw); ++i) {
    uint8_t b = kTextPunctuationRaw[i];
    if (b != 0)
      g_punctuation[b] = i;
  }
}

int32_t CBC_PDF417HighLevelEncoder::EncodeText(WideString msg,
                                               size_t startpos,
                                               size_t count,
                                               WideString& sb,
                                               int32_t initialSubmode) {
  WideString tmp;
  tmp.Reserve(count);
  int32_t submode = initialSubmode;
  size_t idx = 0;
  while (true) {
    wchar_t ch = msg[startpos + idx];
    switch (submode) {
      case SUBMODE_ALPHA:
        if (IsAlphaUpper(ch)) {
          if (ch == ' ')
            tmp += 26;
          else
            tmp += ch - 65;
          break;
        }
        if (IsAlphaLower(ch)) {
          submode = SUBMODE_LOWER;
          tmp += 27;
          continue;
        }
        if (IsMixed(ch)) {
          submode = SUBMODE_MIXED;
          tmp += 28;
          continue;
        }
        tmp += 29;
        tmp += g_punctuation[ch];
        break;
      case SUBMODE_LOWER:
        if (IsAlphaLower(ch)) {
          if (ch == ' ')
            tmp += 26;
          else
            tmp += ch - 97;
          break;
        }
        if (IsAlphaUpper(ch)) {
          tmp += 27;
          tmp += ch - 65;
          break;
        }
        if (IsMixed(ch)) {
          submode = SUBMODE_MIXED;
          tmp += 28;
          continue;
        }

        tmp += 29;
        tmp += g_punctuation[ch];
        break;
      case SUBMODE_MIXED:
        if (IsMixed(ch)) {
          tmp += g_mixed[ch];
          break;
        }
        if (IsAlphaUpper(ch)) {
          submode = SUBMODE_ALPHA;
          tmp += 28;
          continue;
        }
        if (IsAlphaLower(ch)) {
          submode = SUBMODE_LOWER;
          tmp += 27;
          continue;
        }
        if (startpos + idx + 1 < count) {
          wchar_t next = msg[startpos + idx + 1];
          if (IsPunctuation(next)) {
            submode = SUBMODE_PUNCTUATION;
            tmp += 25;
            continue;
          }
        }
        tmp += 29;
        tmp += g_punctuation[ch];
        break;
      default:
        if (IsPunctuation(ch)) {
          tmp += g_punctuation[ch];
          break;
        }
        submode = SUBMODE_ALPHA;
        tmp += 29;
        continue;
    }
    idx++;
    if (idx >= count)
      break;
  }
  wchar_t h = 0;
  size_t len = tmp.GetLength();
  for (size_t i = 0; i < len; i++) {
    bool odd = (i % 2) != 0;
    if (odd) {
      h = (h * 30) + tmp[i];
      sb += h;
    } else {
      h = tmp[i];
    }
  }
  if ((len % 2) != 0)
    sb += (h * 30) + 29;
  return submode;
}

void CBC_PDF417HighLevelEncoder::EncodeBinary(std::vector<uint8_t>* bytes,
                                              size_t startpos,
                                              size_t count,
                                              int32_t startmode,
                                              WideString& sb) {
  if (count == 1 && startmode == TEXT_COMPACTION)
    sb += kShiftToByte;

  size_t idx = startpos;
  if (count >= 6) {
    sb += kLatchToByte;
    wchar_t chars[5];
    while ((startpos + count - idx) >= 6) {
      int64_t t = 0;
      for (size_t i = 0; i < 6; i++) {
        t <<= 8;
        t += (*bytes)[idx + i] & 0xff;
      }
      for (size_t i = 0; i < 5; i++) {
        chars[i] = (t % 900);
        t /= 900;
      }
      for (size_t i = 5; i >= 1; i--)
        sb += (chars[i - 1]);
      idx += 6;
    }
  }
  if (idx < startpos + count)
    sb += kLatchToBytePadded;
  for (size_t i = idx; i < startpos + count; i++) {
    int32_t ch = (*bytes)[i] & 0xff;
    sb += ch;
  }
}

void CBC_PDF417HighLevelEncoder::EncodeNumeric(WideString msg,
                                               size_t startpos,
                                               size_t count,
                                               WideString& sb) {
  size_t idx = 0;
  BigInteger num900 = 900;
  while (idx < count) {
    WideString tmp;
    size_t len = 44 < count - idx ? 44 : count - idx;
    ByteString part = (L'1' + msg.Mid(startpos + idx, len)).UTF8Encode();
    BigInteger bigint = stringToBigInteger(part.c_str());
    do {
      int32_t c = (bigint % num900).toInt();
      tmp += c;
      bigint = bigint / num900;
    } while (!bigint.isZero());
    for (size_t i = tmp.GetLength(); i >= 1; i--)
      sb += tmp[i - 1];
    idx += len;
  }
}

bool CBC_PDF417HighLevelEncoder::IsDigit(wchar_t ch) {
  return ch >= '0' && ch <= '9';
}

bool CBC_PDF417HighLevelEncoder::IsAlphaUpper(wchar_t ch) {
  return ch == ' ' || (ch >= 'A' && ch <= 'Z');
}

bool CBC_PDF417HighLevelEncoder::IsAlphaLower(wchar_t ch) {
  return ch == ' ' || (ch >= 'a' && ch <= 'z');
}

bool CBC_PDF417HighLevelEncoder::IsMixed(wchar_t ch) {
  return g_mixed[ch] != -1;
}

bool CBC_PDF417HighLevelEncoder::IsPunctuation(wchar_t ch) {
  return g_punctuation[ch] != -1;
}

bool CBC_PDF417HighLevelEncoder::IsText(wchar_t ch) {
  return ch == '\t' || ch == '\n' || ch == '\r' || (ch >= 32 && ch <= 126);
}

size_t CBC_PDF417HighLevelEncoder::DetermineConsecutiveDigitCount(
    WideString msg,
    size_t startpos) {
  size_t count = 0;
  size_t len = msg.GetLength();
  size_t idx = startpos;
  if (idx < len) {
    wchar_t ch = msg[idx];
    while (IsDigit(ch) && idx < len) {
      count++;
      idx++;
      if (idx < len)
        ch = msg[idx];
    }
  }
  return count;
}

size_t CBC_PDF417HighLevelEncoder::DetermineConsecutiveTextCount(
    WideString msg,
    size_t startpos) {
  size_t len = msg.GetLength();
  size_t idx = startpos;
  while (idx < len) {
    wchar_t ch = msg[idx];
    size_t numericCount = 0;
    while (numericCount < 13 && IsDigit(ch) && idx < len) {
      numericCount++;
      idx++;
      if (idx < len)
        ch = msg[idx];
    }
    if (numericCount >= 13)
      return idx - startpos - numericCount;
    if (numericCount > 0)
      continue;
    ch = msg[idx];
    if (!IsText(ch))
      break;
    idx++;
  }
  return idx - startpos;
}

Optional<size_t> CBC_PDF417HighLevelEncoder::DetermineConsecutiveBinaryCount(
    WideString msg,
    std::vector<uint8_t>* bytes,
    size_t startpos) {
  size_t len = msg.GetLength();
  size_t idx = startpos;
  while (idx < len) {
    wchar_t ch = msg[idx];
    size_t numericCount = 0;
    while (numericCount < 13 && IsDigit(ch)) {
      numericCount++;
      size_t i = idx + numericCount;
      if (i >= len)
        break;
      ch = msg[i];
    }
    if (numericCount >= 13)
      return idx - startpos;

    size_t textCount = 0;
    while (textCount < 5 && IsText(ch)) {
      textCount++;
      size_t i = idx + textCount;
      if (i >= len)
        break;
      ch = msg[i];
    }
    if (textCount >= 5)
      return idx - startpos;
    ch = msg[idx];
    if ((*bytes)[idx] == 63 && ch != '?')
      return {};
    idx++;
  }
  return idx - startpos;
}
