import argparse

def validate_arguments(l):
    for n in l:
        if n not in range(10):
            return False
    return True

def get_filename(a, c):
    result = "fee_a"

    for id in a:
        result += f"{id}"
    
    result += "_c"

    for id in c:
        result += f"{id}"

    result += ".csv"
    return result

def read_file(file_path):
    with open(file_path, 'r') as f:
        return f.readlines()

def write_file(file_path, lines):
    with open(file_path, 'w') as f:
        f.writelines(lines)

def get_addr(line):
    addr_str, _ = line.split(',', 1)
    return int(addr_str, 16)

def add_to_addr(base, line):
    addr_str, rest = line.split(',', 1)
    addr = int(addr_str, 16)
    new_addr = addr + base
    return f"{new_addr:04x},{rest}"

def shifted_pm_lines(base, pm_lines):
    new_pm_lines = [add_to_addr(base, line) for line in pm_lines]
    new_pm_lines[-1] += '\n'
    return new_pm_lines

def find_insert_index(base, tcm_lines):
    i = 0
    while base > get_addr(tcm_lines[i]):
        i += 1
    return i

def connect_pm(pm_id, tcm_lines, pm_lines):
    base = 0x200 * (pm_id + 1)
    new_pm_lines = shifted_pm_lines(base, pm_lines)
    insert_index = find_insert_index(base, tcm_lines)
    return tcm_lines[:insert_index] + new_pm_lines + tcm_lines[insert_index:]

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate FEE register map")
    parser.add_argument('-a', '--a', nargs='+', type=int, default=[], help="List of connected A side PMs")
    parser.add_argument('-c', '--c', nargs='+', type=int, default=[], help="List of connected C side PMs")
    parser.add_argument('-t', '--tcm_file', nargs=1, type=str, default="tcm.csv", help="Path to TCM register map file")
    parser.add_argument('-p', '--pm_file', nargs=1, type=str, default="pm.csv", help="Path to PM register map file")
    
    args = parser.parse_args()

    if not validate_arguments(args.a) or not validate_arguments(args.c):
        raise ValueError("PM index out of range")
    
    filename = get_filename(args.a, args.c)
    print(f"Generating register map for TCM with PM A: {args.a}, PM C: {args.c} to file {filename}")

    tcm_lines = read_file(args.tcm_file)
    pm_lines = read_file(args.pm_file)

    for id in args.a:
        tcm_lines = connect_pm(id, tcm_lines, pm_lines)
    
    for id in args.c:
        tcm_lines = connect_pm(id + 10, tcm_lines, pm_lines)
    
    write_file(filename, tcm_lines)

    print("Finished generating")
    


    