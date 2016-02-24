PMTCalibration(){

   // Variable declarations
   // NOTE - Make sure the number of elements in Voltage is correct. If its not, this will
   // confuse the forloop.

   float  Voltage[19] = {1.432,1.434,1.436,1.438,1.440,1.442,1.444,1.446,1.448,1.450,1.452,1.454, 1.456, 1.458, 1.460, 1.462, 1.464, 1.466, 1.468}; // LED Voltages
   float  PedestalFitMin = -6e-12;                // Minimum for pedestal fitting range
   float  PedestalFitMax = 2e-8;                  // Maximum for pedestal fitting range
   float  SignalNorm = 100;                       // Normalization for signal fitting
   float  SignalMean = 5e-10;                     // Mean of signal distribution
   float  SignalWidth = 1e-10;                    // Width of signal distribution
   float  MaxADCforPlot = 7e-10;                     // Maximum of x-axis for linearity plot
   float  MaxNPEforPlot = 60;                   // Maximum of y-axis for linearity plot
   string IsPedestalFit;                          // String for user interface
   string IsFullADCFit;                           // String for user interface
   string IsPlotPretty;                           // String for user interface
   int    dummy;                                  // Dummy variable used in sprintf stuff
   char   filename[50];                           // Array of filenames
   char   plotname[50];                           // Array of ADC plot names
   // Ntuple for storing the derived quantities at each voltage
   TNtuple *ntuple = new TNtuple("ntuple","data for each voltage","V:mean:sigma:meanerr:sigmaerr");

   // Begin loop over all files/voltages

   for (unsigned int i=0; i<sizeof(Voltage)/sizeof(Voltage[0]); i++) {

      // To learn about sprintf:
      // http://www.cplusplus.com/reference/cstdio/sprintf/
      // To learn about "%.3f":
      // http://en.wikibooks.org/wiki/C++_Programming/Code/Standard_C_Library/Functions/printf

      dummy=sprintf(filename,"F3squarepmt_1300v_%.3fv00000.txt",Voltage[i]);
      dummy=sprintf(plotname,"F3squarepmt_1300v_%.3fv00000.pdf",Voltage[i]);
      cout<<"filename: "<<filename<<endl;

      // Put contents of text file into TGraph

      TGraph *OrignalADC = new TGraph(filename);
      OrignalADC->Draw("A*");

      // While-loop to continue iterating over the fit until the user approves it
      // To learn about cin and cout:
      // http://www.cplusplus.com/doc/tutorial/basic_io/

      IsPedestalFit = "N";
      while (IsPedestalFit != "Y") {

	 // Fit a gaussian over the pedestal only. Use fit output to shift ADC plot.
         // The 'gpad' lines are necessary so that the plot continues to display while waiting
	 // for user input (https://root.cern.ch/phpBB3/viewtopic.php?f=3&t=18852)

         PedestalFit = new TF1("h1","gaus",PedestalFitMin,PedestalFitMax);
         OrignalADC->Fit(PedestalFit,"R");
         OrignalADC->Draw("A*");
         gPad->Modified();
         gPad->Update();
         gSystem->ProcessEvents();
         cout << "Is the pedestal fit correctly?"<< endl;
         cout << "Return Y for yes and N for no."<< endl;
         cin >> IsPedestalFit;

	 // If pedestal fit is incorrect, change pedestal fit range and try again

         if (IsPedestalFit != "Y") {

            cout << "Enter the minimum fit range for the pedestal (ie -2e10)"<<endl;
            cin >> PedestalFitMin;
            cout << "Enter the minimum fit range for the pedestal (ie 2e10)"<<endl;
            cin >> PedestalFitMax;

         }

      }


      // Define a New TGraph with x-axis rescaled using the function below

      TGraph *ModifiedADC = rescaleaxis(OrignalADC,-1.0,-1.0*PedestalFit->GetParameter(1));

      // Use another while loop to assure that the pedestal plus signal fit is good

      IsFullADCFit = "N";
      while (IsFullADCFit != "Y") {

         // Fit with two gaussians. 0-2 are signal. 3-5 are pedestal. The 'FixParameter' keeps the pedestal gaussian fixed at 0.

         TF1 *FullADCFit = new TF1("FullADCFit","([0]*exp(-0.5*((x-[1])/[2])^2))+([3]*exp(-0.5*((x-[4])/[5])^2))",-1,1);
         FullADCFit->SetParameters(SignalNorm,SignalMean,SignalWidth,PedestalFit->GetParameter(0), 0.0, PedestalFit->GetParameter(2));
         FullADCFit->FixParameter(4,0);
         ModifiedADC->Fit(FullADCFit,"R");
         ModifiedADC->Draw("A*");
         gPad->Modified();
         gPad->Update();
         cout << "Is the full ADC distribution fit correctly?"<< endl;
         cout << "Return Y for yes and N for no."<< endl;
         cin >> IsFullADCFit;

         // If ADC isn't fit correctly, give the user a chance to change the inital values of the signal fit.

         if (IsFullADCFit != "Y") {

            cout << "Enter a new initial value for the signal normalization (ie 100)"<<endl;
            cin >> SignalNorm;
            cout << "Enter a new initial value for the signal mean (ie 5e10)"<<endl;
            cin >> SignalMean;
            cout << "Enter a new initial value for the standard deviation (ie 1e10)"<<endl;
            cin >> SignalWidth;

         }

      }

      // Save each ADC plot and fill the pedestal subtracted mean and sigma (and their respective errors) in ntuple.

      c1->SaveAs(plotname);
      ntuple->Fill(Voltage[i],FullADCFit->GetParameter(1),FullADCFit->GetParameter(2),FullADCFit->GetParError(1),FullADCFit->GetParError(2));

   }

   // We move the contents of ntuple into a TGraphErrors with the first two lines below.
   // See http://root.cern.ch/root/html/TTree.html
   // NPE is stored in TGraphErrors as (mean*mean/(sigma*sigma))
   // I fount TGraphErrors easier to manipulate than TNtuple, especially for plotting error bars.
   // Then we fit a line (pol1) over our plot of NPE vs mean

   ntuple->Draw("mean:(mean*mean/(sigma*sigma)):meanerr:2.0*sqrt((sigmaerr/sigma)**2.0+(meanerr/mean)**2.0)","","*p");
   NPEvsMean = new TGraphErrors(ntuple->GetEntries(),ntuple->GetV1(),ntuple->GetV2(),ntuple->GetV3(),ntuple->GetV4());
   NPEvsMean->SetMarkerStyle(24);
   NPEvsMean->Draw("ap same");
   j1 = new TF1("j1","pol1",0,1);
   NPEvsMean->Fit(j1,"R");

   // Style preferences

   gStyle->SetOptTitle(kFALSE);      // Don't draw title because root often makes those hideous. Easier to add nicely formatted legends.
   gStyle->SetOptStat(0);            // Don't draw stat box
   gStyle->SetTitleSize(0.05,"XY");  // Make axis titles large
   gStyle->SetTitleOffset(.9, "XY"); // Offset axis titles so that they don't interfer with axis
   gStyle->SetOptFit(111);           // Add chi^2 and linear fit values to plot

   // Label above plot so that we aren't confused in 6 months.

   TLegend* legend = new TLegend(0.15,0.91,0.51,0.97,"Square PMT at 1300V","NDC");
   legend->SetTextSize(.05);
   legend->SetFillColor(0);
   legend->SetBorderSize(0);

   // Use one last string to allow the user to keep adjusting axis ranges until all points show up.

   IsPlotPretty = "N";
   while (IsPlotPretty != "Y") {

      // Use a dummy/empty histogram to draw the axis since TH1s are *much* easier to format than TNtuple/TGraphs

      hDummy = new TH2D("hDummy","hDummy",1000,0,MaxADCforPlot,1000,0,MaxNPEforPlot);
      hDummy->GetXaxis()->SetTitle("Pedestal Subtracted ADC");
      hDummy->GetYaxis()->SetTitle("N_{PE}");
      hDummy->GetXaxis()->CenterTitle();
      hDummy->GetYaxis()->CenterTitle();

      hDummy->Draw("axis"); // Draw axis using dummy TH1
      NPEvsMean->Draw("same p");   // Draw TGraph points
      NPEvsMean->Draw("same e");   // Draw TGraph error bars. These are usually too small to show up
      legend->Draw("same"); // Add some informative text

      // Save and add to presentation

      c1->SaveAs("BrandonatorsquareinLinearity.pdf");

      // Have the user approve the plot

      cout << "Is the plot pretty?"<< endl;
      cout << "Return Y for yes and N for no."<< endl;
      cin >> IsPlotPretty;

      // If plot is not approved, adjust axis to make sure we have good ranges

      if (IsPlotPretty != "Y") {

         cout << "Enter a new x-range maximum (ie 3e9)"<<endl;
         cin >> MaxADCforPlot;
         cout << "Enter a new y-range maximum (ie 50)"<<endl;
         cin >> MaxNPEforPlot;

      }

   }

}

// http://root.cern.ch/phpBB3/viewtopic.php?f=3&t=17766&hilit=+axis
// This shifts each x-axis point so that the pedestal sits at 0 then multiplies by a scale
// A scale of -1 flips the x-axis so that signal is right of pedestal on a negative polarity PMT

TGraph* rescaleaxis(TGraph *g,double scale,double shift)
{  

   int N=g->GetN();
   double *y=g->GetY();
   double *x=g->GetX();
   int i=0;
   while(i<N)
   { 
      x[i]=x[i]+shift;
      x[i]=x[i]*scale;
      i=i+1;
   } 
   
   gr = new TGraph(N,x,y);
   return gr;

}
