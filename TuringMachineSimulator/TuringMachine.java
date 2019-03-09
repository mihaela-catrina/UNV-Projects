/*
Nume: Catrina Mihaela-Florentina
Grupa: 323CB
E-mail: catrina.mihaela20@gmail.com
**/

import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.Hashtable;
import java.util.Scanner;

/*
Class that implements turing machine model
**/
public class TuringMachine {
    private int noFinalStates;
    private Transition[] transitionsArray;
    private Hashtable<String, Integer> ht;
    private String[] finalStates;
    private String initialState;
    private String tape1;
    private String tape2;
    private final static int L = -1, R = 1, H = 0;
    private static final String inputFile = "task1.in";

    private TuringMachine() {
    }

    /*
    method to insert value in transitions' hashmap
    * */
    private void insertTransition(String key, int value) {
        ht.put(key, value);
    }

    /*
    Method to create Turing Machine: read input file, parse content and, 
	create transitions' array and hashmap that corresponds
    * */
    private void createTuringMachine() {
        int i = 0;
        String[] s;
        String line;
        try {
            Scanner sc = new Scanner(new FileReader(inputFile));
            line = sc.nextLine();
            s = line.split(" ");
            int noStates = Integer.parseInt(s[0]);
            line = sc.nextLine();
            String[] states = line.split(" ");
            line = sc.nextLine();
            s = line.split(" ");
            noFinalStates = Integer.parseInt(s[0]);
            line = sc.nextLine();
            finalStates = line.split(" ");
            line = sc.nextLine();
            s = line.split(" ");
            initialState = s[0];
            line = sc.nextLine();
            s = line.split(" ");
            int noTransition = Integer.parseInt(s[0]);
            transitionsArray = new Transition[noTransition];
            ht = new Hashtable<>(noTransition);

            while (i < noTransition) {
                line = sc.nextLine();
                s = line.split(" ");
                Transition tr = new Transition(s[3], s[4].charAt(0),
								s[5].charAt(0), s[6].charAt(0), s[7].charAt(0));
                String key = s[0].concat(s[1]).concat(s[2]);
                transitionsArray[i] = tr;
                insertTransition(key, i);
                i++;
            }
            tape1 = sc.nextLine();
            tape1 = tape1.replace(" ", "#");
            tape2 = sc.nextLine();
            tape2 = tape2.replace(" ", "#");
        } catch (IOException e) {
            System.out.println(
                    "Error reading file '"
                            + inputFile + "'");
        }
    }

    /*
    * Method to run TM: goes through the tapes according to the transitions */
    private int runTuringMachine() {
        int i = 1, j = 1;
        int ok = 0;
        String currState = initialState;

        while (i >= 0 && i < tape1.length() && j >= 0 && j < tape2.length()) {
            if (i == tape1.length() - 1) {
                tape1 = tape1 + "#";
            }
            if (j == tape2.length() - 1) {
                tape2 = tape2 + "#";
            }
            //if it reaches one final state => break
            for (int k = 0; k < noFinalStates; k++) {
                if (currState.equals(finalStates[k])) {
                    ok = 1;
                    break;
                }
            }
            if (ok == 1) break;
            char tape1ReadSymbol = tape1.charAt(i);
            char tape2ReadSymbol = tape2.charAt(j);
            // compose key for hashmap
            String key = currState + tape1ReadSymbol + tape2ReadSymbol;
            boolean aux = ht.containsKey(key);
            if (!aux) break;
            //get current transition index in array of transitions
            int index = ht.get(key);
            Transition tr = transitionsArray[index];
            //get directions
            char dir1 = tr.getTape1Direction();
            char dir2 = tr.getTape2Direction();
            char[] tape1Char = tape1.toCharArray();
            char[] tape2Char = tape2.toCharArray();
            tape1Char[i] = tr.tape1WriteSymbol;
            tape2Char[j] = tr.tape2WriteSymbol;
            tape1 = String.valueOf(tape1Char);
            tape2 = String.valueOf(tape2Char);
            //move according to the transition
            if (dir1 == 'H') i = i + H;
            if (dir1 == 'R') i = i + R;
            if (dir1 == 'L') i = i + L;
            if (dir2 == 'H') j = j + H;
            if (dir2 == 'R') j = j + R;
            if (dir2 == 'L') j = j + L;
            currState = tr.nextState;
        }
        // ok = 1 if it reached final state , 0 instead
        return ok;
    }

    public static void main(String[] args) {
        TuringMachine TM = new TuringMachine();
        TM.createTuringMachine();
        int ok = TM.runTuringMachine();
        //write new tapes' content in the output file
        try {
            FileWriter fileWriter = new FileWriter("task1.out");
            PrintWriter printWriter = new PrintWriter(fileWriter);
            if (ok == 0) printWriter.println("The machine has blocked!");
            else {
                printWriter.println(TM.tape1);
                printWriter.println(TM.tape2);
            }
            printWriter.close();

        } catch (IOException e) {
            System.out.println("Error creating output file");
        }
    }
}
