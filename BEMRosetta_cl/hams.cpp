#include "BEMRosetta.h"
#include "BEMRosetta_int.h"
#include <STEM4U/Utility.h>


Vector<String> HamsCal::Check() {
	Vector<String> ret;
	
	ret.Append(BemCal::Check(true));
	
	if (bodies.size() != 1)
		ret << t_("HAMS just allows one body");
	else {
		bool y0zmesh, x0zmesh;
		{
			MeshData data;
			String err = data.LoadPnlHAMS(bodies[0].meshFile, y0zmesh, x0zmesh);
			if (!err.IsEmpty()) {
				ret << err;
				return ret;
			}
		}
		if (!bodies[0].lidFile.IsEmpty()) {
			bool y0zlid, x0zlid;
			{
				MeshData data;
				String err = data.LoadPnlHAMS(bodies[0].lidFile, y0zlid, x0zlid);
				if (!err.IsEmpty()) {
					ret << err;
					return ret;
				}
			}
			if (y0zmesh != y0zlid)
				ret << t_("The symmetry of the X-axis (y0z) in the hull and the lid has to match");	
			if (x0zmesh != x0zlid)
				ret << t_("The symmetry of the Y-axis (x0z) in the hull and the lid has to match");
		}
	}
	return ret;
}

bool HamsCal::Load(String fileName) {
	FileInLine in(fileName);
	if (!in.IsOpen())
		return false;
		
	String line;
	FieldSplit f(in);
	f.IsSeparator = IsTabSpace;
	
	int input_frequency_type = 0, output_frequency_type = 0;
	
	while (!f.IsEof()) {
		f.LoadLine();
		
		if (f.GetText().Find("End HAMS Control file") >= 0)
			break;
		if (f.size() < 2)
			continue;
		String var = Trim(f.GetText(0));
		if (var.StartsWith("#"))
			continue;
		if (var == "Waterdepth")
			h = f.GetDouble(1);
		else if (var == "Input_frequency_type") {
			input_frequency_type = f.GetInt(1); 
			if (input_frequency_type != 3 && input_frequency_type != 4)
				throw Exc("HAMS loader just allows loading input_frequency_type = 3 wave frequency or 4 wave period");
		} else if (var == "Output_frequency_type") {
			output_frequency_type = f.GetInt(1);
			if (output_frequency_type != 3 && output_frequency_type != 4)
				throw Exc("HAMS loader just allows loading output_frequency_type = 3 wave frequency or 4 wave period");
		} else if (var == "Number_of_frequencies") {
			Nf = f.GetInt(1);
			if (Nf < 0) {
				Nf = -Nf;
				f.LoadLine();
				if (f.size() < 2 || Trim(f.GetText(0)) != "Minimum_frequency_Wmin")
					throw Exc("Minimum_frequency_Wmin not found");
				minF = f.GetDouble(1);
				f.LoadLine();
				if (f.size() < 2 || Trim(f.GetText(0)) != "Frequency_step")
					throw Exc("Frequency_step not found");
				maxF = minF + f.GetDouble(1)*Nf;
			} else {
				f.LoadLine();
				if (f.size() < 2)
					throw Exc("Frequencies or periods not found");
				Vector<double> data;
				double delta;
				for (int i = 0; i < f.size(); ++i) {
					if (input_frequency_type == 3)
						data << f.GetDouble(i);
					else
						data.At(0, 2*M_PI/f.GetDouble(i));
					if (i == 1)
						delta = data[i] - data[i-1];
					else if (i > 1 && !EqualDecimals(delta, data[i] - data[i-1], 3))
						throw Exc("HAMS loader just allows equidistant frequencies");
				}
				minF = data[0];
				maxF = data[data.size()-1];
			}
		} else if (var == "Number_of_headings") {
			Nh = f.GetInt(1);
			if (Nh < 0) {
				Nh = -Nh;
				f.LoadLine();
				if (f.size() < 2 || Trim(f.GetText(0)) != "Minimum_heading")
					throw Exc("Minimum_heading not found");
				minH = f.GetDouble(1);
				f.LoadLine();
				if (f.size() < 2 || Trim(f.GetText(0)) != "Heading_step")
					throw Exc("Heading_step not found");
				maxH = minH + f.GetDouble(1)*Nh;
			} else {
				f.LoadLine();
				if (f.size() < 2)
					throw Exc("Headings not found");
				Vector<double> data;
				double delta;
				for (int i = 0; i < f.size(); ++i) {
					data << f.GetDouble(i);
					if (i == 1)
						delta = data[i] - data[i-1];
					else if (i > 1 && !EqualDecimals(delta, data[i] - data[i-1], 3))
						throw Exc("HAMS loader just allows equidistant headings");
				}
				minH = data[0];
				maxH = data[data.size()-1];
			}
		} else if (var == "Reference_body_center") {
			if (f.size() < 4)
				throw Exc("Lack of data in Reference_body_center");
			bodies.SetCount(1);
			BemBody &body = bodies[0];
			body.c0[0] = f.GetDouble(1);
			body.c0[1] = f.GetDouble(2);
			body.c0[2] = f.GetDouble(3);
			String meshFile = AppendFileName(GetFileFolder(fileName), "HullMesh.pnl");
			if (FileExists(meshFile))
				body.meshFile = meshFile;
			String lidFile = AppendFileName(GetFileFolder(fileName), "WaterplaneMesh.pnl");
			if (FileExists(lidFile))
				body.lidFile = lidFile;
		}
	}
	return LoadHydrostatic(AppendFileName(GetFileFolder(fileName), "Hydrostatic.in"));
}

bool HamsCal::LoadHydrostatic(String fileName) {
	FileInLine in(fileName);
	if (!in.IsOpen())
		return false;
	
	bodies.SetCount(1);
	BemBody &body = bodies[0];
	
	String line;
	FieldSplit f(in);
	f.IsSeparator = IsTabSpace;
	
	while (!f.IsEof()) {
		f.LoadLine();
		
		if (f.size() < 1)
			continue;	
		
		String line = Trim(f.GetText());
	
		if (line == "Center of Gravity:") {
			f.LoadLine();
			if (f.size() < 3)
				throw Exc("Center of Gravity data is not complete");
			body.cg[0] = f.GetDouble(0);
			body.cg[1] = f.GetDouble(1);
			body.cg[2] = f.GetDouble(2);
		} else if (line == "Body Mass Matrix:") 
			InMatrix(f, body.mass);
		else if (line == "External Linear Damping Matrix:") 
			InMatrix(f, body.linearDamping);
		else if (line == "External Quadratic Damping Matrix:") 
			InMatrix(f, body.quadraticDamping);	
		else if (line == "Hydrostatic Restoring Matrix:") 
			InMatrix(f, body.hydrostaticRestoring);	
		else if (line == "External Restoring Matrix:") 
			InMatrix(f, body.externalRestoring);	
	}
	return true;
}

void HamsCal::SaveFolder(String folderBase, bool bin, int numCases, int numThreads, const BEMData &bem) const {
	SaveFolder0(folderBase, bin, 1, bem, true, numThreads);
	if (numCases > 1)
		SaveFolder0(folderBase, bin, numCases, bem, false, numThreads);
}

void HamsCal::SaveFolder0(String folderBase, bool bin, int numCases, const BEMData &bem, bool deleteFolder, int numThreads) const {
	BeforeSave(folderBase, numCases, deleteFolder);
	
	#define MIN_F_HAMS 0.01
	
	double fixminF = minF;
	if (fixminF < MIN_F_HAMS)
		fixminF = MIN_F_HAMS;
	
	Vector<int> valsf;
	int _nf;
	double _minf, _maxf;
	int ifr = 0;
	Vector<double> freqs;
	if (numCases > 1) { 
		LinSpaced(freqs, Nf, fixminF, maxF);
		valsf = NumSets(Nf, numCases);
	}
	
	String solvName = "HAMS_x64.exe";
	if (bin) {
		String source = AppendFileName(bem.hamsPath, solvName);
		String destNew = AppendFileName(folderBase, solvName);
		if (!FileCopy(source, destNew)) 
			throw Exc(Format(t_("Problem copying Hams exe file from '%s'"), bem.hamsPath));
		source = AppendFileName(bem.hamsPath, "libiomp5md.dll");		
		destNew = AppendFileName(folderBase, "libiomp5md.dll");		
		if (!FileCopy(source, destNew)) 
			throw Exc(Format(t_("Problem copying Hams dll file from '%s'"), source));					
	} 
		
	//String sumcases;
	for (int i = 0; i < numCases; ++i) {
		String folder;
		if (numCases > 1) {
			folder = AppendFileName(folderBase, Format("HAMS_Part_%d", i+1));
			if (!DirectoryCreateX(folder))
				throw Exc(Format(t_("Problem creating '%s' folder"), folder));
			//sumcases << " " << AppendFileName(folder, "Nemoh.cal");
			_minf = freqs[ifr];
			int deltaf = valsf[i];
			_maxf = freqs[ifr + deltaf - 1];
			_nf = deltaf;
			ifr += deltaf;
		} else {
			folder = folderBase;
			_nf = Nf;
			_minf = fixminF;
			_maxf = maxF;
		}
		String folderInput = AppendFileName(folder, "Input");
		if (!DirectoryCreateX(folderInput))
			throw Exc(Format(t_("Problem creating '%s' folder"), folderInput));
		
		Save_ControlFile(folderInput, _nf, _minf, _maxf, numThreads);
		Save_Hydrostatic(folderInput);
	
		int ib = 0;		// Just one file
		{
			String dest = AppendFileName(folderInput, "HullMesh.pnl");
			if (GetFileExt(bodies[ib].meshFile) == ".pnl") {
				if (!FileCopy(bodies[ib].meshFile, dest)) 
					throw Exc(Format(t_("Problem copying mesh file from '%s'"), bodies[ib].meshFile));
			} else {
				MeshData mesh;
				String err = mesh.Load(bodies[ib].meshFile);
				if (!err.IsEmpty())
					throw Exc(err);
				mesh.SavePnlHAMS(dest, mesh.mesh, false, false);
			}
		}
		if (!bodies[ib].lidFile.IsEmpty()) {
			String dest = AppendFileName(folderInput, "WaterplaneMesh.pnl");
			if (GetFileExt(bodies[ib].lidFile) == ".pnl") {
				if (!FileCopy(bodies[ib].lidFile, dest)) 
					throw Exc(Format(t_("Problem copying lid file from '%s'"), bodies[ib].lidFile));
			} else {
				MeshData mesh;
				String err = mesh.Load(bodies[ib].lidFile);
				if (!err.IsEmpty())
					throw Exc(err);
				mesh.SavePnlHAMS(dest, mesh.mesh, false, false);
			}
		}
		String folderOutput = AppendFileName(folder, "Output");
		if (!DirectoryCreateX(folderOutput))
			throw Exc(Format(t_("Problem creating '%s' folder"), folderOutput));
		if (!DirectoryCreateX(AppendFileName(folderOutput, "Hams_format")))
			throw Exc(Format(t_("Problem creating '%s' folder"), AppendFileName(folderOutput, "Hams_format")));
		if (!DirectoryCreateX(AppendFileName(folderOutput, "Hydrostar_format")))
			throw Exc(Format(t_("Problem creating '%s' folder"), AppendFileName(folderOutput, "Hydrostar_format")));
		if (!DirectoryCreateX(AppendFileName(folderOutput, "Wamit_format")))
			throw Exc(Format(t_("Problem creating '%s' folder"), AppendFileName(folderOutput, "Wamit_format")));
		
		if (numCases > 1) 
			Save_Bat(folderBase, Format("HAMS_Part_%d.bat", i+1), Format("HAMS_Part_%d", i+1), bin, solvName);
		else
			Save_Bat(folder, "HAMS.bat", Null, bin, solvName);
	}
}

void HamsCal::Save_Bat(String folder, String batname, String caseFolder, bool bin, String solvName) const {
	String fileName = AppendFileName(folder, batname);
	FileOut out(fileName);
	if (!out.IsOpen())
		throw Exc(Format(t_("Impossible to create '%s'"), fileName));
	if (!IsNull(caseFolder))
		out << "cd \"" << caseFolder << "\"\n";
	out << "\"" << solvName << "\"\n";
}

void HamsCal::OutMatrix(FileOut &out, String header, const Eigen::MatrixXd &mat) {
	out << "\n " << header << ":";
	for (int y = 0; y < 6; ++y) {
		out << "\n";
		for (int x = 0; x < 6; ++x)
			out << Format("   %0.5E", mat(x, y));
	}
}

void HamsCal::InMatrix(FieldSplit &f, Eigen::MatrixXd &mat) {
	for (int y = 0; y < 6; ++y) {
		f.LoadLine();
		for (int x = 0; x < 6; ++x)
			mat(x, y) = f.GetDouble(x);
	}
}
	
void HamsCal::Save_Hydrostatic(String folderInput) const {
	String fileName = AppendFileName(folderInput, "Hydrostatic.in");
	FileOut out(fileName);
	if (!out.IsOpen())
		throw Exc(Format(t_("Impossible to create '%s'"), fileName));
	out << " Center of Gravity:";
	
	if (bodies.IsEmpty())
		throw Exc(t_("No bodies found"));
	
	const BemBody &b = bodies[0];
	out << Format("\n %0.15E %0.15E %0.15E", b.cg[0], b.cg[1], b.cg[2]);

	OutMatrix(out, "Body Mass Matrix", b.mass);
	OutMatrix(out, "External Linear Damping Matrix", b.linearDamping);
	OutMatrix(out, "External Quadratic Damping Matrix", b.quadraticDamping);
	OutMatrix(out, "Hydrostatic Restoring Matrix", b.hydrostaticRestoring);
	OutMatrix(out, "External Restoring Matrix", b.externalRestoring);
}

void HamsCal::Save_ControlFile(String folderInput, int _nf, double _minf, double _maxf,
					int numThreads) const {
	String fileName = AppendFileName(folderInput, "ControlFile.in");
	FileOut out(fileName);
	if (!out.IsOpen())
		throw Exc(Format(t_("Impossible to create '%s'"), fileName));	
	
	out << "   --------------HAMS Control file---------------"
		   "\n";
	out << "\n   Waterdepth  " << Format("%.4f", h) << "D0";
	out << "\n"
		   "\n   #Start Definition of Wave Frequencies"
		   "\n    Input_frequency_type        3"
		   "\n    Output_frequency_type       3";
	out << "\n    Number_of_frequencies      " << _nf;
	Vector<double> freqs;
	LinSpaced(freqs, _nf, _minf, _maxf);
	out << "\n    ";
	for (const auto &freq : freqs)
		out << Format("%.4f ", freq);	
	out << "\n   #End Definition of Wave Frequencies"
		   "\n"
		   "\n   #Start Definition of Wave Headings";
	out << "\n    Number_of_headings         " << Nh;
	Vector<double> headings;
	LinSpaced(headings, Nh, minH, maxH);
	out << "\n    ";
	for (const auto &heading : headings)
		out << Format("%.4f ", heading);	
	out << "\n   #End Definition of Wave Headings"
		   "\n";
	out << "\n    Reference_body_center   0.000   0.000   0.000"
		   "\n    Reference_body_length   1.D0"
		   "\n    Wave_diffrac_solution    2";
	bool remove_irr_freq = !bodies[0].lidFile.IsEmpty();
	out << "\n    If_remove_irr_freq      " << (remove_irr_freq ? 1 : 0);
	out << "\n    Number of threads       " << numThreads;
	out << "\n"
		   "\n   #Start Definition of Pressure and/or Elevation (PE)"
		   "\n    Number_of_field_points     1                           # number of field points where to calculate PE"
		   "\n 0.000000    0.000000    0.000000    Global_coords_point_1"
		   "\n   #End Definition of Pressure and/or Elevation"
   		   "\n";
	out << "\n    ----------End HAMS Control file---------------"
		   "\n   Input_frequency_type options:"
		   "\n   1--deepwater wave number; 2--finite-depth wave number; 3--wave frequency; 4--wave period; 5--wave length"
   		   "\n   Output_frequency_type options: same as Input_frequency_type options";
}