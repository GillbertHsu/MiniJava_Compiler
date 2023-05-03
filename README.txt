This project introduce code optimization

I first calculate USE DEF of each 3ac instruction, and link the successor and predecessor while
traversing the instructions vector. After that, I calculate the Liveness of each 3ac variable.
I calculate method by method. For each method, start from the end and push the last instruction
into stack. In the loop, I pop the instruction from the stack and calculate the liveness of it.
I use set_union adn set_difference to calculate the liveness based on the equation in the lecture.
Then I distinguish whether the instruction already reach the begin of the method. If not, I push
the instruction before the current instruction into the stack. Another special case is the branch,
if I detect the instruction has 2 predecessors, I push the instruction before the current instruction
into the stack then I push the instruction of its predecessor that are below the current instruction
into the stack. After that, I build graph based on the liveness of each instruction. Then I push 
those node into coloring stack based on its degree. I use the algorithm in the lecture to color
the graph. After that, I use the color to replace the variable in the instruction.
