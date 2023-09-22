import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.lang.Math;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.stream.Stream;

public class simulator {
    public static void main(String[] args) throws IOException {
//        calculateConfig();
//        calculateConfig_test();

        //Setup checking all .out files
        final File folder = new File("./simulation_outs/");
        String dir = "./simulation_outs/";

        //Arrlist for all file names
        ArrayList<String> names = new ArrayList<>();
        ArrayList<String> gcc = new ArrayList<>();
        ArrayList<String> mcf = new ArrayList<>();
        ArrayList<String> perl = new ArrayList<>();
        ArrayList<String> x264 = new ArrayList<>();
        //Method to get all file names
        listFilesForFolder(folder, names);

        //Two arrays to store best predictions and the config settings
        //0: Weighted, 1: gcc, 2: mcf, 3: perl, 4: x264
        String[] bestName = new String[5];
        double[] bestPrediction = new double[5];
        double[] tempStore = new double[4];
        double[] forBestWeighted = new double[4];
        //Temp double to calculate weighted Prediction
        double weightedPrediction = 0;

        for (int i = 0; i < names.size(); i++) {
            String prediction = "";
            //Get accuracy of name[i]
            try (Stream<String> lines = Files.lines(Paths.get(dir + names.get(i)))) {
                prediction = lines.skip(16).findFirst().get().substring(36);
            }
            if (i % 4 == 0) {
                if (bestPrediction[0] < weightedPrediction) {
                    if (i != 0) {
                        bestName[0] = names.get(i - 1).substring(0,18);
                        forBestWeighted[0] = tempStore[0];
                        forBestWeighted[1] = tempStore[1];
                        forBestWeighted[2] = tempStore[2];
                        forBestWeighted[3] = tempStore[3];
                    }
                    bestPrediction[0] = weightedPrediction;
                }
                weightedPrediction = 0;
            }
            weightedPrediction += 0.25 * Double.parseDouble(prediction);

            int predIndex = getPredIndex(names, i);
            if (bestPrediction[predIndex] < Double.parseDouble(prediction)) {
                bestName[predIndex] = names.get(i);
                bestPrediction[predIndex] = Double.parseDouble(prediction);
            }
            tempStore[predIndex - 1] = Double.parseDouble(prediction);

            if (predIndex == 1) {
                gcc.add("" + names.get(i) + ": " + prediction);
            } else if (predIndex == 2) {
                mcf.add("" + names.get(i) + ": " + prediction);
            } else if (predIndex == 3) {
                perl.add("" + names.get(i) + ": " + prediction);
            } else if (predIndex == 4) {
                x264.add("" + names.get(i) + ": " + prediction);
            }
        }

        try {
            FileWriter myWriter = new FileWriter("_output.txt");
            myWriter.write("Weighted:     " + bestName[0] + "                  - " + (1 - bestPrediction[0]) + "\n" +
                    "Gcc:             " + bestName[1] + " - " + (1 - bestPrediction[1]) + "\n" +
                    "Mcf:             " + bestName[2] + " - " + (1 - bestPrediction[2]) + "\n" +
                    "Perl:             " + bestName[3] + " - " + (1 - bestPrediction[3]) + "\n" +
                    "x264:            " + bestName[4] + " - " + (1 - bestPrediction[4]) + "\n\n" +
                    "For best: " + "\n" + "Gcc - " + (1 - forBestWeighted[0]) + " Mcf - " + (1 - forBestWeighted[1]) +
                    " Perl - " + (1 - forBestWeighted[2]) + " x264 - " + (1- forBestWeighted[3]));
            myWriter.close();
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }

        try {
            FileWriter gccWriter = new FileWriter("./_gcc.txt");
            FileWriter mcfWriter = new FileWriter("./_mcf.txt");
            FileWriter perlWriter = new FileWriter("./_perl.txt");
            FileWriter x264Writer = new FileWriter("./_x264.txt");
            for (int i = 0; i < gcc.size(); i++) {
                gccWriter.write(gcc.get(i) + "\n");
                mcfWriter.write(mcf.get(i) + "\n");
                perlWriter.write(perl.get(i) + "\n");
                x264Writer.write(x264.get(i) + "\n");
            }
            gccWriter.close();
            mcfWriter.close();
            perlWriter.close();
            x264Writer.close();
        } catch (IOException e) {
            System.out.println("An error occurred.");
            e.printStackTrace();
        }

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
        for (int c = 0; c < 4; c++) {
            for (int p = 9; p < 32; p++) {
                for (int l = 9; l < 32; l++) {
                    for (int n = 8; n < 32; n++) {
                        for (int g = 32; g < 65; g++) {
                            double Yp = p * Math.pow(2, l) + 2 * Math.pow(2, p);
                            double Perc = (Math.ceil(Math.log(1.93 * g + 14) / Math.log(2)) + 1) * (g + 1) * Math.pow(2, n);
                            if (Yp >= 3 * Math.pow(2, 14) && Perc >= 3 * Math.pow(2, 14) && Yp + Perc <= 3 * Math.pow(2, 16)) {
                                config.add("PLNGC_" + p + "_" + l + "_" + n + "_" + g + "_" + c);
                                System.out.println("flags_" + "PLNGC_" + p + "_" + l + "_" + n + "_" + g + "_" + c + "='-M 3 -P " + p + " -L " + l + " -N " + n + " -G " + g + " -C " + c + "'");
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
        System.out.print(")\n");
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
