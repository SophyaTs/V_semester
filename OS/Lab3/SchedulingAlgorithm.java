// Run() is called from Scheduling.main() and is where
// the scheduling algorithm written by the user resides.
// User modification should occur within the Run() function.

import java.util.Vector;
import java.util.Queue;
import java.util.LinkedList;
import java.io.*;

public class SchedulingAlgorithm {

	public static void progress(sProcess process, int timeUsed) {
		process.cpudone+=timeUsed;
	}
	
	public static void progressIO(sProcess process, int timeUsed) {
		process.cpudone+=timeUsed;
		process.ionext+=process.ioblocking;
		++process.numblocked;  
	}
	
  public static Results Run(int runtime, Vector processVector, Results result) {
    //int i = 0;
    int comptime = 0;
    int currentProcess = 0;
    sProcess process = null;
    Queue<Integer> queue = new LinkedList<Integer>();
    final int quantum = 30;
    int actuallyUsed = 0;
    //int previousProcess = 0;
    int size = processVector.size();
    //int completed = 0;
    String resultsFile = "Summary-Processes";

    result.schedulingType = "Interactive (Nonpreemptive)";
    result.schedulingName = "Round-Robin"; 
    try {
      //BufferedWriter out = new BufferedWriter(new FileWriter(resultsFile));
      //OutputStream out = new FileOutputStream(resultsFile);
      PrintStream out = new PrintStream(new FileOutputStream(resultsFile));
      //sProcess process = (sProcess) processVector.elementAt(currentProcess);
      
      for(int i=0; i < size;++i) {
    	  process = (sProcess) processVector.elementAt(i);
    	  process.ionext = process.ioblocking;
    	  queue.add(i);
      }
      
      while (comptime < runtime) {
    	  if(queue.isEmpty()) //all processes have been completed
    		  break;
    	  currentProcess = queue.poll();    	  
    	  
    	  process = (sProcess) processVector.elementAt(currentProcess);
    	  out.println("Process: " + currentProcess + " gets quantum... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
    	  
    	  if(process.cputime - process.cpudone <= quantum) { // process may be completed in this quantum
    		  if(process.ionext <= process.cputime) { // process gets IO blocked before completion
    			  actuallyUsed = process.ionext - process.cpudone;
    			  progressIO(process, actuallyUsed);
    			  queue.add(currentProcess);
    			  out.println("Process: " + currentProcess + " I/O blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
    		  } 
    		  else { // process actually finishes
    			  actuallyUsed = process.cputime - process.cpudone;
    			  progress(process, actuallyUsed);
    			  out.println("Process: " + currentProcess + " completed... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
    		  }
    	  }
    	  else {
    		  if(process.ionext <= process.cpudone + quantum) {
    			  actuallyUsed = process.ionext - process.cpudone;
    			  progressIO(process, actuallyUsed);
    			  queue.add(currentProcess);
    			  out.println("Process: " + currentProcess + " I/O blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
    		  }
    		  else {
    			  actuallyUsed = quantum;
    			  progress(process, actuallyUsed);
    			  ++process.numblocked;
    			  queue.add(currentProcess);
    			  out.println("Process: " + currentProcess + " blocked... (" + process.cputime + " " + process.ioblocking + " " + process.cpudone + ")");
    		  }
    	  }
    	  
    	  comptime+=actuallyUsed;
      }
      out.close();
    } catch (IOException e) { /* Handle exceptions */ }
    result.compuTime = comptime;
    return result;
  }
}
