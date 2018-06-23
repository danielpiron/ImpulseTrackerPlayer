#include <array>
#include <fstream>
#include <iostream>
#include <vector>

struct PatternEntry {
    class Note {
        static const uint8_t empty = 253;
        static const uint8_t cut = 254;
        static const uint8_t off = 255;

    public:
        bool is_empty() const { return _index == empty; }
        bool is_cut() const { return _index == cut; }
        bool is_off() const { return _index == off; }
        int octave() const { return _index / 12; }
        int semitone() const { return _index % 12; }
        int period() const
        {
            static const short periods[] = {
                1712,
                1616,
                1524,
                1440,
                1356,
                1280,
                1208,
                1140,
                1076,
                1016,
                960,
                907
            };
            return 16 * periods[semitone()] / octave();
        }
        operator int() const { return _index; }
        Note() = default;
        explicit Note(const uint8_t i)
            : _index(i)
        {
        }

    private:
        uint8_t _index = empty;
    };
    class Inst {
    public:
        static const uint8_t empty = 255;
        Inst() = default;
        explicit Inst(const uint8_t i)
            : _index(i)
        {
        }

    private:
        uint8_t _index = empty;
    };
    class Command {
    public:
        enum class Type : uint8_t {
            none,
            set_speed,
            set_tempo,
            set_volume,
            set_panning
        };
        Command() = default;
        explicit Command(Type t, uint8_t v)
            : _type(t)
            , _value(v)
        {
        }

    private:
        Type _type = Type::none;
        uint8_t _value = 0;
    };
    using Comms = std::array<Command, 2>;

    Note note;
    Inst inst;
    Comms comms;
};

class Pattern {
    static const size_t max_channels = 64;
    static const size_t default_rows = 64;
    using RowType = std::array<PatternEntry, max_channels>;

public:
    void set(const PatternEntry& entry, size_t row, size_t col)
    {
        rows[row].entries[col] = entry;
    }
    const RowType& row(size_t r) const { return rows[r].entries; }
    explicit Pattern(size_t n_rows = default_rows)
        : rows(n_rows)
    {
    }

private:
    struct Row {
        RowType entries;
    };
    std::vector<Row> rows;
};

namespace it_file {
#pragma pack(push, 1)
struct header {
    char impm[4]; // Must be 'I', 'M', 'P', 'M'
    char song_name[26];
    uint16_t philiht; // Pattern row highlight information. Only relevant for pattern editing situations.
    uint16_t order_num;
    uint16_t instrument_num;
    uint16_t sample_num;
    uint16_t pattern_num;
    uint16_t created_with;
    uint16_t compatible_with;
    uint16_t flags;
    uint16_t special;
    uint8_t global_volume; // 0->128
    uint8_t mix_volume; // 0->128
    uint8_t initial_speed;
    uint8_t initial_tempo;
    uint8_t panning_separation;
    uint8_t pitch_wheel_depth;
    uint16_t message_length;
    uint32_t message_offset;
    uint32_t reserved;
    uint8_t channel_panning[64];
    uint8_t channel_volume[64];
};
#pragma pack(pop)
}

template<typename T>
std::vector<T> load_vector(std::istream &f, const size_t count) {
    std::vector<T> temp(count);
    f.read(reinterpret_cast<char*>(&temp[0]), static_cast<std::streamsize>(count * sizeof(T)));
    return std::move(temp);
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::ifstream it("/home/piron/Downloads/m4v-fasc.it", std::ios::binary);
    it_file::header it_header;
    it.read(reinterpret_cast<char*>(&it_header), sizeof it_header);

    auto orders = load_vector<uint8_t>(it, it_header.order_num);
    auto instrument_offsets = load_vector<uint32_t>(it, it_header.instrument_num);
    auto sample_offsets = load_vector<uint32_t>(it, it_header.sample_num);
    auto pattern_offsets = load_vector<uint32_t>(it, it_header.pattern_num);

    Pattern p;
    std::cin >> argc;

    return 0;
}
