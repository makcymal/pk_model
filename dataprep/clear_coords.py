with open('pk_model/graph_coord_population.txt', 'r') as file:
    lines = []
    for line in file:
        line = line.replace('\t', ' ')
        lines.append(' '.join([line[:line.find(' ')], line[line.rfind(' ') + 1:]]))

with open('pk_model/graph_popul.txt', 'w') as file:
    for line in lines:
        file.write(line)