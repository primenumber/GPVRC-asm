#include <cctype>
#include <algorithm>
#include <bitset>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <png++/png.hpp>

enum class TokenType {
  Empty,
  Identifier,
  Number,
  BracketL,
  BracketR,
  Period,
  Invalid,
};

std::ostream& operator<<(std::ostream& os, const TokenType type) {
  switch (type) {
    case TokenType::Empty:      return os << "TokenType::Empty";
    case TokenType::Identifier: return os << "TokenType::Identifier";
    case TokenType::Number:     return os << "TokenType::Number";
    case TokenType::BracketL:   return os << "TokenType::BracketL";
    case TokenType::BracketR:   return os << "TokenType::BracketR";
    case TokenType::Period:     return os << "TokenType::Period";
    case TokenType::Invalid:    return os << "TokenType::Invalid";
    default:                    return os << "[Unknown Token Type]";
  }
}

struct Token {
  TokenType type;
  std::string str;
};

std::ostream& operator<<(std::ostream& os, const Token& token) {
  return os << "[ " << token.type << "; " << token.str << " ]";
}

void parse_error_unexpected_token_type(const Token& got, const std::vector<TokenType>& expected_types, const size_t line_number) {
  using std::size;
  std::cerr << "Parse error: unexpected token at line " << line_number << ". expected";
  size_t N = size(expected_types);
  if (N == 1) {
    std::cerr << " " << expected_types.front();
  } else {
    for (size_t i = 0; i < N; ++i) {
      std::cerr << " " << expected_types[i];
      if (i < N-2) {
        std::cerr << ",";
      } else if (i == N-2) {
        std::cerr << " or";
      }
    }
  }
  std::cerr << ", but got " << got.type << std::endl;
}

TokenType is_prefix_of_token(const std::string& str) {
  using std::size;
  if (str.empty()) return TokenType::Empty;
  unsigned char front = static_cast<unsigned char>(str.front());
  if (front == '.') return size(str) == 1 ? TokenType::Period : TokenType::Invalid;
  if (front == '[') return size(str) == 1 ? TokenType::BracketL : TokenType::Invalid;
  if (front == ']') return size(str) == 1 ? TokenType::BracketR : TokenType::Invalid;
  if (std::isalpha(front)) {
    return std::all_of(begin(str), end(str),
        [](const char& ch) { return std::isalnum(static_cast<unsigned char>(ch)); }
    ) ? TokenType::Identifier : TokenType::Invalid;
  }
  if (std::isdigit(front)) {
    return std::all_of(begin(str), end(str),
        [](const char& ch) { return std::isdigit(static_cast<unsigned char>(ch)); }
    ) ? TokenType::Number : TokenType::Invalid;
  }
  return TokenType::Invalid;
}

std::vector<Token> tokenize(const std::string& line) {
  std::vector<Token> tokens;
  std::string current;
  for (auto&& ch : line) {
    current.push_back(ch);
    if (is_prefix_of_token(current) == TokenType::Invalid) {
      current.pop_back();
      auto type = is_prefix_of_token(current);
      if (type != TokenType::Empty && type != TokenType::Invalid) {
        tokens.push_back({ type, current });
      }
      current.clear();
      current.push_back(ch);
      if (is_prefix_of_token(current) == TokenType::Invalid) {
        // ignore current charactor
        current.clear();
      }
    }
  }
  if (!current.empty()) {
    auto type = is_prefix_of_token(current);
    tokens.push_back({ type, current });
  }
  return tokens;
}

using Instruction = std::bitset<24>;
constexpr size_t general_reg_count = 16;

Instruction generate_reg1_imm16(uint32_t opcode, uint32_t reg, uint32_t imm) {
  assert(opcode < 0xC);
  assert(reg < general_reg_count);
  assert(imm < 0x10000);
  return (opcode << 20) | (reg << 16) | imm;
}

Instruction generate_reg2_imm8(uint32_t opcode, uint32_t reg_1, uint32_t reg_2, uint32_t imm) {
  assert(opcode < 0x10);
  assert(reg_1 < general_reg_count);
  assert(reg_2 < general_reg_count);
  assert(imm < 0x100);
  return 0xC0'00'00 | (opcode << 16) | (reg_1 << 12) | (reg_2 << 8) | imm;
}

Instruction generate_imm16(uint32_t opcode, uint32_t imm) {
  assert(opcode < 0x10);
  assert(imm < 0x10000);
  return 0xD0'00'00 | (opcode << 16) | imm;
}

Instruction generate_reg3(uint32_t opcode, uint32_t reg_dest, uint32_t reg_src1, uint32_t reg_src2) {
  assert(opcode < 0x100);
  assert(reg_dest < general_reg_count);
  assert(reg_src1 < general_reg_count);
  assert(reg_src2 < general_reg_count);
  return 0xE0'00'00 | (opcode << 12) | (reg_dest << 8) | (reg_src1 << 4) | reg_src2;
}

Instruction generate_reg1_imm8(uint32_t opcode, uint32_t reg, uint32_t imm) {
  assert(opcode < 0x80);
  assert(reg < general_reg_count);
  assert(imm < 0x100);
  return 0xF0'00'00 | (opcode << 12) | (reg << 8) | imm;
}

Instruction generate_reg2(uint32_t opcode, uint32_t reg_1, uint32_t reg_2) {
  assert(opcode < 0x400);
  assert(reg_1 < general_reg_count);
  assert(reg_2 < general_reg_count);
  return 0xF8'00'00 | (opcode << 8) | (reg_1 << 4) | reg_2;
}

Instruction generate_imm8(uint32_t opcode, uint32_t imm) {
  assert(opcode < 0x200);
  assert(imm < 0x100);
  return 0xFC'00'00 | (opcode << 8) | imm;
}

Instruction generate_reg1(uint32_t opcode, uint32_t reg) {
  assert(opcode < 0x1000);
  assert(reg < general_reg_count);
  return 0xFE'00'00 | (opcode << 4) | reg;
}

Instruction generate_no_operands(uint32_t opcode) {
  assert(opcode < 0x10000);
  return 0xFF'00'00 | opcode;
}

template <typename Iter>
uint32_t parse_reg(Iter& first, Iter last, const size_t line_number) {
  using std::size;
  using std::begin;
  using std::end;
  if (first == last) {
    std::cerr << "Unexpected EOL at line " << line_number << ", expected Identifier" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  auto token = *first++;
  if (token.type != TokenType::Identifier) {
    parse_error_unexpected_token_type(token, { TokenType::Identifier }, line_number);
    std::exit(EXIT_FAILURE);
  }
  const auto& reg_str = token.str;
  if (size(reg_str) < 2) {
    std::cerr << "Unexpected Identifier at line " << line_number << ", expected Register, but got " << reg_str << std::endl;
  }
  if (reg_str.front() != 'r') {
    std::cerr << "Unexpected Identifier at line " << line_number << ", expected Register, but got " << reg_str << std::endl;
  }
  if (std::any_of(std::next(begin(reg_str)), end(reg_str),
        [] (const char& ch) { return !std::isdigit(static_cast<unsigned char>(ch)); })) {
    std::cerr << "Unexpected Identifier at line " << line_number << ", expected Register, but got " << reg_str << std::endl;
  }
  uint32_t reg_id = std::stoul(reg_str.substr(1));
  if (reg_id >= general_reg_count) {
    std::cerr << "Too large Register ID at line " << line_number << ", expected 0 - 15, but got " << reg_id << std::endl;
  }
  return reg_id;
}

template <typename Iter>
uint32_t parse_imm(Iter& first, Iter last, const std::map<std::string, size_t>& labels, const size_t line_number) {
  using std::size;
  using std::begin;
  using std::end;
  if (first == last) {
    std::cerr << "Unexpected EOL at line " << line_number << ", expected Number or Period" << std::endl;
    std::exit(EXIT_FAILURE);
  }
  auto token = *first++;
  if (token.type == TokenType::Number) {
    return std::stoul(token.str);
  } else if (token.type == TokenType::Period) {
    if (first == last) {
      std::cerr << "Unexpected EOL at line " << line_number << ", expected Identifier" << std::endl;
      std::exit(EXIT_FAILURE);
    }
    auto token_second = *first++;
    if (token_second.type != TokenType::Identifier) {
      parse_error_unexpected_token_type(token, { TokenType::Identifier }, line_number);
      std::exit(EXIT_FAILURE);
    }
    auto itr = labels.find(token_second.str);
    if (itr == end(labels)) {
      std::cerr << "Undefined label at line " << line_number << ": " << token_second.str << std::endl;
      std::exit(EXIT_FAILURE);
    }
    return itr->second;
  } else {
    parse_error_unexpected_token_type(token, { TokenType::Number, TokenType::Period }, line_number);
    std::exit(EXIT_FAILURE);
  }
}

enum class OperandsType {
  Reg1Imm16,
  Reg2Imm8,
  Imm16,
  Reg3,
  Reg1Imm8,
  Reg2,
  Imm8,
  Reg1,
  Empty,
  Invalid,
};

template <typename Iter>
Instruction generate(
    const OperandsType op_type,
    const uint32_t op_code,
    Iter first, Iter last,
    const std::map<std::string, size_t>& labels,
    const size_t line_number) {
  using std::size;
  if (op_type == OperandsType::Reg1Imm16) {
    auto reg = parse_reg(first, last, line_number);
    auto imm = parse_imm(first, last, labels, line_number);
    return generate_reg1_imm16(op_code, reg, imm);
  } else if (op_type == OperandsType::Reg2Imm8) {
    auto reg1 = parse_reg(first, last, line_number);
    auto reg2 = parse_reg(first, last, line_number);
    auto imm = parse_imm(first, last, labels, line_number);
    return generate_reg2_imm8(op_code, reg1, reg2, imm);
  } else if (op_type == OperandsType::Imm16) {
    auto imm = parse_imm(first, last, labels, line_number);
    return generate_imm16(op_code, imm);
  } else if (op_type == OperandsType::Reg3) {
    auto reg1 = parse_reg(first, last, line_number);
    auto reg2 = parse_reg(first, last, line_number);
    auto reg3 = parse_reg(first, last, line_number);
    return generate_reg3(op_code, reg1, reg2, reg3);
  } else if (op_type == OperandsType::Reg1Imm8) {
    auto reg = parse_reg(first, last, line_number);
    auto imm = parse_imm(first, last, labels, line_number);
    return generate_reg1_imm8(op_code, reg, imm);
  } else if (op_type == OperandsType::Reg2) {
    auto reg1 = parse_reg(first, last, line_number);
    auto reg2 = parse_reg(first, last, line_number);
    return generate_reg2(op_code, reg1, reg2);
  } else if (op_type == OperandsType::Imm8) {
    auto imm = parse_imm(first, last, labels, line_number);
    return generate_imm8(op_code, imm);
  } else if (op_type == OperandsType::Reg1) {
    auto reg = parse_reg(first, last, line_number);
    return generate_reg1(op_code, reg);
  } else if (op_type == OperandsType::Empty) {
    return generate_no_operands(op_code);
  } else {
    std::cerr << "Unknown OperandsType: " << static_cast<size_t>(op_type) << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

Instruction assemble_impl(
    const std::vector<Token>& tokens,
    const std::map<std::string, size_t>& labels,
    const size_t line_number) {
  using std::size;
  using std::begin;
  using std::end;
  if (size(tokens) < 1) {
    std::cerr << "Unexpected EOL at line " << line_number << std::endl;
    std::exit(EXIT_FAILURE);
  }
  if (tokens.front().type != TokenType::Identifier) {
    parse_error_unexpected_token_type(tokens.front(), { TokenType::Identifier }, line_number);
    std::exit(EXIT_FAILURE);
  }
  auto op = tokens.front().str;
  OperandsType op_type = OperandsType::Invalid;
  uint32_t op_code = 0;
  if (op == "add") {
    op_type = OperandsType::Reg3;
    op_code = 0x00;
  } else if (op == "sub") {
    op_type = OperandsType::Reg3;
    op_code = 0x01;
  } else if (op == "umul") {
    op_type = OperandsType::Reg3;
    op_code = 0x02;
  } else if (op == "imul") {
    op_type = OperandsType::Reg3;
    op_code = 0x03;
  } else if (op == "udiv") {
    op_type = OperandsType::Reg3;
    op_code = 0x04;
  } else if (op == "umod") {
    op_type = OperandsType::Reg3;
    op_code = 0x06;
  } else if (op == "addi") {
    op_type = OperandsType::Reg2Imm8;
    op_code = 0x00;
  } else if (op == "subi") {
    op_type = OperandsType::Reg2Imm8;
    op_code = 0x01;
  } else if (op == "shli") {
    op_type = OperandsType::Reg2Imm8;
    op_code = 0x04;
  } else if (op == "shri") {
    op_type = OperandsType::Reg2Imm8;
    op_code = 0x05;
  } else if (op == "load") {
    op_type = OperandsType::Reg2;
    op_code = 0x000;
  } else if (op == "store") {
    op_type = OperandsType::Reg2;
    op_code = 0x001;
  } else if (op == "jez") {
    op_type = OperandsType::Reg2;
    op_code = 0x010;
  } else if (op == "jnz") {
    op_type = OperandsType::Reg2;
    op_code = 0x011;
  } else if (op == "not") {
    op_type = OperandsType::Reg2;
    op_code = 0x100;
  } else if (op == "neg") {
    op_type = OperandsType::Reg2;
    op_code = 0x101;
  } else if (op == "cid") {
    op_type = OperandsType::Reg1;
    op_code = 0x000;
  } else if (op == "jmp") {
    op_type = OperandsType::Reg1;
    op_code = 0x010;
  } else if (op == "loadi") {
    op_type = OperandsType::Reg1Imm16;
    op_code = 0x0;
  } else if (op == "jezi") {
    op_type = OperandsType::Reg1Imm16;
    op_code = 0x2;
  } else if (op == "jnzi") {
    op_type = OperandsType::Reg1Imm16;
    op_code = 0x3;
  } else if (op == "jmpi") {
    op_type = OperandsType::Imm16;
    op_code = 0x0;
  } else if (op == "exit") {
    op_type = OperandsType::Empty;
    op_code = 0xffff;
  } else {
    std::cerr << "Unknown OpName at line " << line_number << ": " << op << std::endl;
  }
  return generate(op_type, op_code, std::next(begin(tokens)), end(tokens), labels, line_number);
  return std::bitset<24>(0);
}

std::vector<Instruction> assemble(std::istream& is) {
  using std::size;
  using inst_lines = std::tuple<size_t, std::vector<Token>>;
  std::vector<inst_lines> instruction_tokens;
  std::map<std::string, size_t> labels;
  constexpr size_t line_length_max = 256;
  size_t line_number = 1;
  for (std::string line; std::getline(is, line); ++line_number) {
    if (size(line) > line_length_max) {
      std::cerr << "Too long line length at line " << line_number << std::endl;
      std::exit(EXIT_FAILURE);
    }
    const auto tokens = tokenize(line);
    if (tokens.empty()) continue;
    else if (tokens.front().type == TokenType::Period) {
      if (size(tokens) < 2) {
        std::cerr << "Unexpected EOL at line " << line_number << ", Expected identifier" << std::endl;
        std::exit(EXIT_FAILURE);
      }
      labels[tokens.at(1).str] = size(instruction_tokens);
    } else {
      instruction_tokens.emplace_back(line_number, std::move(tokens));
    }
  }
  std::vector<Instruction> insts;
  for (auto&& [line_number, tokens] : instruction_tokens) {
    insts.push_back(assemble_impl(tokens, labels, line_number));
  }
  return insts;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Error: Missing command line options" << std::endl;
    std::cerr << "Usage: " << argv[0] << " [ASM_FILE] [BIN_FILE]" << std::endl;
    return EXIT_FAILURE;
  }
  std::ifstream ifs(argv[1]);
  if (!ifs.is_open()) {
    std::cerr << "Failed to open file: " << argv[1] << std::endl;
    return EXIT_FAILURE;
  }
  auto binary = assemble(ifs);
  constexpr size_t image_size = 256;
  png::image<png::rgb_pixel> image(image_size, image_size);
  for (size_t i = 0; i < std::size(binary); ++i) {
    auto y = image_size - 1 - i / image_size;
    auto x = i % image_size;
    png::rgb_pixel pixel(
         binary[i].to_ulong()        & 0xff,
        (binary[i].to_ulong() >>  8) & 0xff,
        (binary[i].to_ulong() >> 16) & 0xff);
    image.set_pixel(x, y, pixel);
  }
  image.write(argv[2]);
  return 0;
}
