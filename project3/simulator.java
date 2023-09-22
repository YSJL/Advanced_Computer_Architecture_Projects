import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.Math;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;
import java.util.Collections;
import java.util.Map;
import java.util.HashMap;

public class simulator {
    public static void main(String[] args) throws IOException {
//        calculateConfig();
//        calculateConfig_test();

        //Setup checking all .out files
        final File folder = new File("./tiled_outs/");
        String dir = "./tiled_outs/";
//
//        //Arrlist for all file names
        ArrayList<String> names = new ArrayList<>();
        ArrayList<String> nameList = new ArrayList<>();
        ArrayList doubleList = new ArrayList<Double>();
        ArrayList oneList = new ArrayList<Double>();
        ArrayList twoList = new ArrayList<Double>();
        ArrayList threeList = new ArrayList<Double>();
        ArrayList fourList = new ArrayList<Double>();
        ArrayList fiveList = new ArrayList<Double>();

        double one = 1.48;
        double two = 1.477;
        double three = 1.476;
        double four = 1.474;
        double five = 1.473;


//        ArrayList<String> bfs = new ArrayList<>();
//        ArrayList<String> cachesim = new ArrayList<>();
//        ArrayList<String> perceptron = new ArrayList<>();
//        ArrayList<String> tiled = new ArrayList<>();
//        //Method to get all file names
        listFilesForFolder(folder, names);
//
//        //Two arrays to store best predictions and the config settings
//        //0: Weighted, 1: gcc, 2: mcf, 3: perl, 4: x264
//        String[] bestName = new String[5];
//        double[] bestPrediction = new double[5];
//        double[] tempStore = new double[4];
//        double[] forBestWeighted = new double[4];
//        //Temp double to calculate weighted Prediction
//        double weightedPrediction = 0;
//
        for (int i = 0; i < names.size(); i++) {
            String ipcLine = "";

            //Get accuracy of name[i]
            try (Stream<String> lines = Files.lines(Paths.get(dir + names.get(i)))) {
                ipcLine = lines.skip(31).findFirst().orElse("");

                double ipcValue = Double.parseDouble(ipcLine.split("\\s+")[1]);
                //System.out.println(ipcLine);
                nameList.add(names.get(i));
                doubleList.add(ipcValue);
                if (ipcValue == one) {
                    oneList.add(names.get(i));
                } else if (ipcValue == two) {
                    twoList.add(names.get(i));
                } else if (ipcValue == three) {
                    threeList.add(names.get(i));
                } else if (ipcValue == four) {
                    fourList.add(names.get(i));
                } else if (ipcValue == five) {
                    fiveList.add(names.get(i));
                }

            } catch (IOException | NumberFormatException e) {
                System.out.println("Error: " + e.getMessage());
            }

        }

//        System.out.println("Sorted double list: " + sorteddoubleList);
//        System.out.println("Sorted string list: " + sortednameList);
//
        try {
            FileWriter myWriter = new FileWriter("_tiled_1_output.txt");
            myWriter.write("Top Three:" + "\n"
              + "1. " + one + ": " + oneList + "\n"
                    + "2. " + two + ": " + twoList + "\n"
                    + "3. " + three + ": " + threeList + "\n"
                    + "4. " + four + ": " + fourList + "\n"
                    + "5. " + five + ": " + fiveList + "\n");
            myWriter.close();
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }
//
//        try {
//            FileWriter gccWriter = new FileWriter("./_gcc.txt");
//            FileWriter mcfWriter = new FileWriter("./_mcf.txt");
//            FileWriter perlWriter = new FileWriter("./_perl.txt");
//            FileWriter x264Writer = new FileWriter("./_x264.txt");
//            for (int i = 0; i < gcc.size(); i++) {
//                gccWriter.write(gcc.get(i) + "\n");
//                mcfWriter.write(mcf.get(i) + "\n");
//                perlWriter.write(perl.get(i) + "\n");
//                x264Writer.write(x264.get(i) + "\n");
//            }
//            gccWriter.close();
//            mcfWriter.close();
//            perlWriter.close();
//            x264Writer.close();
//        } catch (IOException e) {
//            System.out.println("An error occurred.");
//            e.printStackTrace();
//        }

//        System.out.println("Weighted:     " + bestName[0] + "             - " + bestPrediction[0] + "\n" +
//                "Gcc:          " + bestName[1] + " - " + bestPrediction[1] + "\n" +
//                "Mcf:          " + bestName[2] + " - " + bestPrediction[2] + "\n" +
//                "Perl:         " + bestName[3] + " - " + bestPrediction[3] + "\n" +
//                "x264:         " + bestName[4] + " - " + bestPrediction[4] + "\n\n" +
//                "For best: " + "\n" + "Gcc - " + forBestWeighted[0] + " Mcf - " + forBestWeighted[1] +
//                " Perl - " + forBestWeighted[2] + " x264 - " + forBestWeighted[3]);
    }

    private static void calculateConfig() {
        ArrayList<String> config = new ArrayList<>();
        ArrayList<int[]> settings = new ArrayList<>();
        int[] s = {2,4,8};
        int[] p = {64,96,128};
        int[] f = {2,4,8};
        for (int a = 1; a < 4; a++) {
            for (int m = 1; m < 3; m++) {
                for (int l = 1; l < 4; l++) {
                    for (int i = 0; i < s.length; i++) {
                        for (int j = 0; j < f.length; j++) {
                            for (int k = 0; k < p.length; k++) {
                                config.add("PFSAML_"+ p[k] + "_" + f[j] + "_" + s[i] + "_" + a + "_" + m + "_" + l);
                                int [] temp = {p[k], f[j], s[i], a, m, l};
                                settings.add(temp);
                            }
                        }
                    }
                }
            }
        }
        System.out.print("configs=( ");
        for (int i = 0; i < config.size(); i++) {
            System.out.print(config.get(i) + " ");
        }
        System.out.print(")\n\n");

        for (int i = 0; i < config.size(); i++) {
            System.out.println("flags_" + config.get(i) + "='-P " + settings.get(i)[0] + " -F " + settings.get(i)[1]
              + " -S " + settings.get(i)[2] + " -A " + settings.get(i)[3] + " -M " + settings.get(i)[4] + " -L "
              + settings.get(i)[5] + "'");

        }
    }

    private static void calculateConfig_test() {
        int p = 11;
        int l = 12;
        int n = 9;
        int g = 40;
        boolean correct = false;
        double Yp = p * Math.pow(2, l) + 2 * Math.pow(2, p);
        double Perc = (Math.ceil(Math.log(1.93 * g + 14) / Math.log(2)) + 1) * (g + 1) * Math.pow(2, n);
        double total = Yp + Perc;
        System.out.println("Total: " + total);
        if (Yp >= 3 * Math.pow(2, 14) && Perc >= 3 * Math.pow(2, 14) && Yp + Perc <= 3 * Math.pow(2, 16)) correct = true;
        System.out.println("Correct?: " + correct);
    }
    private static int getPredIndex(ArrayList<String> names, int index) {
        int predIndex = 0;
        if (names.get(index).charAt(18) == 'g' || names.get(index).charAt(19) == 'g') {
            predIndex = 1;
        } else if (names.get(index).charAt(18) == 'm' || names.get(index).charAt(19) == 'm') {
            predIndex = 2;
        } else if (names.get(index).charAt(18) == 'p' || names.get(index).charAt(19) == 'p') {
            predIndex = 3;
        } else if (names.get(index).charAt(18) == 'x' || names.get(index).charAt(19) == 'x') {
            predIndex = 4;
        }
        return predIndex;
    }

    public static void listFilesForFolder(final File folder, ArrayList<String> names) {
        for (final File fileEntry : folder.listFiles()) {
            if (fileEntry.isDirectory()) {
                listFilesForFolder(fileEntry, names);
            } else {
                names.add(fileEntry.getName());
                //System.out.println(fileEntry.getName());
            }
        }
    }
}
