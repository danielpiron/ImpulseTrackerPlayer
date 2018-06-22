#include <array>
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

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    Pattern p;
    std::cin >> argc;

    return 0;
}
