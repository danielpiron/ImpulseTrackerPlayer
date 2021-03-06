#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

struct PatternEntry {
    class Note {
        static const uint8_t empty = 253;
        static const uint8_t cut = 254;
        static const uint8_t off = 255;

    public:
        std::string to_string() const
        {
            if (is_empty()) {
                return "...";
            }
            if (is_cut()) {
                return "===";
            }
            if (is_off()) {
                return "---";
            }
            static const std::string note_names[] = {
                "C-", "C#", "D-", "D#", "E-", "F-", "F#", "G-", "G#", "A-", "A#", "B-"
            };
            return note_names[semitone()] + std::to_string(octave());
        }
        bool is_note() const { return _index < 190; }
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
            return 32 * periods[semitone()] >> octave();
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
        operator int() const { return _index; }
        bool is_empty() const { return _index == empty; }
        explicit Inst(const uint8_t i)
            : _index(i)
        {
        }
        std::string to_string() const
        {
            if (is_empty()) {
                return "..";
            }
            std::stringstream ss;
            ss << std::setfill('0') << std::setw(2) << static_cast<int>(_index);
            return ss.str();
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
            set_panning,
            unknown
        };

        std::string to_string() const
        {
            char type_indicator = '\0';
            switch (_type) {
            case Type::none:
                type_indicator = '.';
                break;
            case Type::set_speed:
                type_indicator = 'A';
                break;
            case Type::set_tempo:
                type_indicator = 'T';
                break;
            case Type::set_volume:
                type_indicator = 'v';
                break;
            case Type::set_panning:
                type_indicator = 'X';
                break;
            case Type::unknown:
                type_indicator = '?';
                break;
            }
            std::stringstream ss;
            ss << type_indicator;
            ss << std::setfill('0') << std::setw(2)
               << std::hex << std::uppercase << static_cast<int>(_param);
            return ss.str();
        }
        bool is_type(Type t) const { return _type == t; }
        uint8_t param() const { return _param; }
        uint8_t param_hi_nybble() const { return _param >> 4; }
        uint8_t param_lo_nybble() const { return _param & 15; }
        Command() = default;
        explicit Command(Type t, uint8_t p)
            : _type(t)
            , _param(p)
        {
        }

    private:
        Type _type = Type::none;
        uint8_t _param = 0;
    };
    using Comms = std::array<Command, 2>;
    std::string to_string() const { return note.to_string() + ' ' + inst.to_string() + ' ' + comms[0].to_string() + ' ' + comms[1].to_string(); }

    Note note;
    Inst inst;
    Comms comms;
};

class Pattern {
    static const size_t max_channels = 64;
    static const size_t default_rows = 64;
    using RowType = std::array<PatternEntry, max_channels>;

public:
    void set(size_t row, size_t col, const PatternEntry& entry)
    {
        rows[row].entries[col] = entry;
    }
    const PatternEntry entry(size_t row, size_t col) const
    {
        return rows[row].entries[col];
    }
    size_t row_count() const { return rows.size(); }
    const RowType& row(size_t r) const { return rows[r].entries; }
    Pattern(Pattern&& rhs)
        : rows(std::move(rhs.rows))
    {
    }
    explicit Pattern(size_t n_rows = default_rows)
        : rows(n_rows)
    {
    }

private:
    struct Row {
        RowType entries;
        Row() = default;
        Row(Row&& rhs)
            : entries(std::move(rhs.entries))
        {
        }
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

struct pattern_header {
    uint16_t packed_data_length;
    uint16_t row_num;
    uint8_t filler[4];
};
#pragma pack(pop)
}

template <typename T>
void flex_read(T* buffer, const size_t count, std::istream& f)
{
    f.read(reinterpret_cast<char*>(&buffer[0]), static_cast<std::streamsize>(count * sizeof(T)));
}

template <typename T>
std::vector<T> load_vector(std::istream& f, const size_t count)
{
    std::vector<T> temp(count);
    flex_read(&temp[0], count, f);
    return std::move(temp);
}

Pattern unpack_pattern(std::istream& f)
{
    it_file::pattern_header pat_header;
    flex_read(&pat_header, 1, f);

    Pattern p;
    uint8_t row = 0;
    uint8_t mask_variables[64];
    PatternEntry entries[64];
    while (row < pat_header.row_num) {
        uint8_t channel_variable = f.get();
        if (channel_variable == 0) {
            row++;
            continue;
        }
        uint8_t mask_variable = 0;
        uint8_t channel = (channel_variable - 1) & 63;
        if (channel_variable & 128) {
            mask_variable = f.get();
            mask_variables[channel] = mask_variable;
        } else {
            mask_variable = mask_variables[channel];
        }
        if (mask_variable & 1) {
            entries[channel].note = PatternEntry::Note(f.get());
        }
        if (mask_variable & 2) {
            entries[channel].inst = PatternEntry::Inst(f.get());
        }
        if (mask_variable & 4) {
            uint8_t vol_comm = f.get();
            if (vol_comm <= 64) {
                entries[channel].comms[0] = PatternEntry::Command(PatternEntry::Command::Type::set_volume, vol_comm);
            } else if (vol_comm >= 128 && vol_comm <= 192) {
                entries[channel].comms[0] = PatternEntry::Command(PatternEntry::Command::Type::set_panning, vol_comm - 65);
            }
        }
        if (mask_variable & 8) {
            uint8_t it_command = f.get();
            PatternEntry::Command::Type type;
            switch (it_command) {
            case 0: // None
                type = PatternEntry::Command::Type::none;
                break;
            case 1: // A - Set Speed
                type = PatternEntry::Command::Type::set_speed;
                break;
            case 20: // T - Set Tempo
                type = PatternEntry::Command::Type::set_tempo;
                break;
            default:
                type = PatternEntry::Command::Type::unknown;
                break;
            }
            entries[channel].comms[1] = PatternEntry::Command(type, f.get());
        }
        PatternEntry entry;
        if (mask_variable & (16 | 1)) {
            entry.note = entries[channel].note;
        }
        if (mask_variable & (32 | 2)) {
            entry.inst = entries[channel].inst;
        }
        if (mask_variable & (64 | 4)) {
            entry.comms[0] = entries[channel].comms[0];
        }
        if (mask_variable & (128 | 8)) {
            entry.comms[1] = entries[channel].comms[1];
        }
        p.set(row, channel, entry);
    }
    return p;
}

struct Module {
    std::string song_name;
    std::vector<uint8_t> orders;
    std::vector<Pattern> patterns;
};

struct PlayerContext {
    struct HostChannel {
        int sample_index = 0;
        int period = 0;
        int volume = 0;
        bool new_note = false;
    };

    const Module* mod;
    std::array<HostChannel, 64> host_channels;
    uint8_t ticks_to_next_row;
    uint8_t current_row;
    uint8_t breaking_row;
    uint8_t current_order;
    uint8_t ticks_per_row; // aka "speed"
    uint8_t tempo;

    const Pattern& current_pattern() const
    {
        return mod->patterns[mod->orders[current_order]];
    }
    void process_row();
    void process_tick();
    void advance_to_next_order();
    PlayerContext(const Module* m)
        : mod(m)
        , ticks_to_next_row(0)
        , current_row(0)
        , breaking_row(64)
        , current_order(0)
        , ticks_per_row(6)
        , tempo(128)
    {
    }
};

void PlayerContext::advance_to_next_order()
{
    while (mod->orders[++current_order] == 254) {
    }
    if (mod->orders[current_order] == 255
        || current_order >= mod->orders.size()) {
        current_order = 0;
    }
}

void PlayerContext::process_row()
{
    size_t c = 0;
    for (const auto& entry : current_pattern().row(current_row)) {
        if (entry.note.is_note()) {
            host_channels[c].period = entry.note.period();
            host_channels[c].sample_index = entry.inst;
            host_channels[c].volume = entry.comms[0].is_type(PatternEntry::Command::Type::set_volume)
                ? entry.comms[0].param()
                : 64; // This should sample default
            host_channels[c].new_note = true;
        }
        c++;
    }
}

void PlayerContext::process_tick()
{
    if (ticks_to_next_row == 0) {
        process_row();
        if (++current_row >= breaking_row) {
            advance_to_next_order();
            current_row = 0;
            breaking_row = current_pattern().row_count();
        }
        ticks_to_next_row = ticks_per_row;
    }
    --ticks_to_next_row;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::ifstream it("/home/piron/Downloads/m4v-fasc.it", std::ios::binary);
    it_file::header it_header;
    it.read(reinterpret_cast<char*>(&it_header), sizeof it_header);

    Module mod;
    mod.orders = load_vector<uint8_t>(it, it_header.order_num);
    auto instrument_offsets = load_vector<uint32_t>(it, it_header.instrument_num);
    auto sample_offsets = load_vector<uint32_t>(it, it_header.sample_num);
    auto pattern_offsets = load_vector<uint32_t>(it, it_header.pattern_num);

    mod.song_name = it_header.song_name;
    mod.patterns.reserve(it_header.pattern_num);
    for (const auto& offset : pattern_offsets) {
        if (offset) {
            it.seekg(offset);
            mod.patterns.emplace_back(unpack_pattern(it));
        } else {
            mod.patterns.emplace_back(Pattern());
        }
    }

    PlayerContext player(&mod);
    const auto& pattern = player.current_pattern();
    for (size_t i = 0; i < 64; i++) {
        for (size_t j = 0; j < 8; j++) {
            std::cout << pattern.entry(i, j).to_string() << "|";
        }
        std::cout << "\n";
    }

    while (true) {
        player.process_tick();
        for (auto& host_channel : player.host_channels) {
            if (host_channel.new_note) {
                std::cout << "Playing Sample #" << host_channel.sample_index
                          << " at period: " << host_channel.period
                          << " and volume: " << host_channel.volume << "\n";
                host_channel.new_note = false;
            }
        }
    }

    std::cin >> argc;

    return 0;
}
