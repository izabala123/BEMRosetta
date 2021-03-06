#include <CtrlLib/CtrlLib.h>
#include <Controls4U/Controls4U.h>
#include <ScatterCtrl/ScatterCtrl.h>
#include <GLCanvas/GLCanvas.h>
#include <RasterPlayer/RasterPlayer.h>
#include <TabBar/TabBar.h>

#include <BEMRosetta_cl/BEMRosetta.h>

using namespace Upp;

#define IMAGECLASS ImgNemoh
#define IMAGEFILE <BEMRosetta/main.iml>
#include <Draw/iml.h>

#include "main.h"


void MainSolver::Init(const BEMData &bem) {
	CtrlLayout(bodies);
	bodiesScroll.AddPaneV(bodies).SizePos();
	CtrlLayout(nemoh);
	CtrlLayout(hams);
	CtrlLayout(*this);
	
	tab.Add(nemohScroll.AddPaneV(nemoh).SizePos(), "Nemoh");
	//tab.Add(hamsScroll.AddPaneV(hams).SizePos(),   "HAMS");
	
	loadFrom.WhenChange = THISBACK(OnLoad);
	loadFrom.BrowseRightWidth(40).UseOpenFolder().BrowseOpenFolderWidth(10).UseDropping();
	butLoad.WhenAction = [&] {loadFrom.DoGo();};

	saveTo.BrowseRightWidth(40).UseOpenFolder().BrowseOpenFolderWidth(10).UseDropping();
	butSave.WhenAction = [&] {OnSave(bem);};
	
	bodies.array.WhenSel = THISBACK(arrayOnCursor);
	
	bodies.butAdd <<= THISBACK(arrayOnAdd);
	bodies.butDuplicate <<= THISBACK(arrayOnDuplicate);
	bodies.butRemove <<= THISBACK(arrayOnRemove);
	bodies.butAllDOF.WhenAction = [&] {
		bodies.surge <<= true; 	
		bodies.sway <<= true; 	
		bodies.heave <<= true; 
		bodies.roll <<= true; 	
		bodies.pitch <<= true; 
		bodies.yaw <<= true;
		ArrayUpdateCursor();
	};
	
	const String meshFiles = ".gdf .dat .stl .pnl";
	String meshFilesAst = clone(meshFiles);
	meshFilesAst.Replace(".", "*.");
	
	bodies.meshFile.WhenAction 	= [&] {ArrayUpdateCursor();};
	bodies.meshFile.WhenChange 	= [&] {return ArrayUpdateCursor();};
	bodies.meshFile.Type(Format("All supported mesh files (%s)", meshFiles), meshFilesAst);
	bodies.meshFile.AllFilesType();
	bodies.lidFile.WhenAction  	= [&] {ArrayUpdateCursor();};
	bodies.lidFile.WhenChange  	= [&] {return ArrayUpdateCursor();};
	bodies.lidFile.Type(Format("All supported mesh files (%s)", meshFiles), meshFilesAst);
	bodies.lidFile.AllFilesType();
	
	bodies.xcm <<= 0;
	bodies.ycm <<= 0;
	bodies.zcm <<= 0;
	
	bodies.surge.WhenAction 	= [&] {ArrayUpdateCursor();};
	bodies.sway.WhenAction 		= [&] {ArrayUpdateCursor();};
	bodies.heave.WhenAction 	= [&] {ArrayUpdateCursor();};
	bodies.roll.WhenAction 		= [&] {ArrayUpdateCursor();};
	bodies.pitch.WhenAction 	= [&] {ArrayUpdateCursor();};
	bodies.yaw.WhenAction 		= [&] {ArrayUpdateCursor();};
	bodies.cx.WhenAction 		= [&] {ArrayUpdateCursor();};
	bodies.cy.WhenAction 		= [&] {ArrayUpdateCursor();};
	bodies.cz.WhenAction 		= [&] {ArrayUpdateCursor();};
	
	nemoh.freeSurface.Transparent(false);
	nemoh.freeSurface.WhenAction = [&] {
		nemoh.freeX.Enable(~nemoh.freeSurface);	
		nemoh.freeY.Enable(~nemoh.freeSurface);
	};
	nemoh.freeSurface.WhenAction();
	
	//HamsCal hams;
	//Load(hams);	
	
	dropSolver.Add(BemCal::NEMOH, t_("Nemoh"));
	dropSolver.Add(BemCal::NEMOHv115, t_("Nemoh 115+"));
	dropSolver.Add(BemCal::CAPYTAINE, t_("Capytaine"));
	dropSolver.Add(BemCal::HAMS, t_("HAMS"));
	dropSolver.SetIndex(dropSolverVal);
	dropSolver.WhenAction = [&] {
		bool isNemoh = ~dropSolver != BemCal::HAMS;
		if (isNemoh)
			tab.Set(nemohScroll);
		//else
		//	tab.Set(hamsScroll);
		
		numThreads.Enable(!isNemoh);
		labnumThreads.Enable(!isNemoh);
		nemohScroll.Enable(isNemoh);
		opIncludeBin.Enable(~dropSolver != BemCal::CAPYTAINE);
		
		bodies.array.HeaderObject().ShowTab(1, !isNemoh);
		bodies.array.HeaderObject().ShowTab(2, isNemoh);
		bodies.array.HeaderObject().ShowTab(3, isNemoh);
		bodies.array.HeaderObject().ShowTab(4, isNemoh);
		bodies.array.HeaderObject().ShowTab(5, isNemoh);
		bodies.array.HeaderObject().ShowTab(6, isNemoh);
		bodies.array.HeaderObject().ShowTab(7, isNemoh);
		bodies.array.HeaderObject().ShowTab(8, isNemoh);
		bodies.array.HeaderObject().ShowTab(9, isNemoh);
		
		bodies.lidFile.Enable(!isNemoh);
		bodies.butAllDOF.Enable(isNemoh);
		bodies.surge.Enable(isNemoh);
		bodies.sway.Enable(isNemoh);
		bodies.heave.Enable(isNemoh);
		bodies.roll.Enable(isNemoh);
		bodies.pitch.Enable(isNemoh);
		bodies.yaw.Enable(isNemoh);
		bodies.xcm.Enable(!isNemoh);
		bodies.ycm.Enable(!isNemoh);
		bodies.zcm.Enable(!isNemoh);
		bodies.mass.Enable(!isNemoh);
		bodies.linearDamping.Enable(!isNemoh);
		bodies.quadraticDamping.Enable(!isNemoh);
		bodies.hydrostaticRestoring.Enable(!isNemoh);
		bodies.externalRestoring.Enable(!isNemoh);
	};
	dropSolver.WhenAction();
	
	opInfinite.WhenAction = [&] {
		height.Enable(!bool(~opInfinite));
	};
	opInfinite.WhenAction();
}

void MainSolver::InitSerialize(bool ret) {
	if (!ret || IsNull(opIncludeBin)) 
		opIncludeBin = true;
	if (!ret || IsNull(numSplit)) 	
		numSplit = 2;	
	String manufacturer, productName, version, mbSerial;
	int numberOfProcessors;
	GetSystemInfo(manufacturer, productName, version, numberOfProcessors, mbSerial);
	if (!ret || IsNull(numThreads)) 	
		numThreads = numberOfProcessors;
	if (!ret || IsNull(~nemoh.xeff))
		nemoh.xeff <<= 0;
	if (!ret || IsNull(~nemoh.yeff))
		nemoh.yeff <<= 0;
	if (!ret || IsNull(~bodies.cx))
		bodies.cx <<= 0;
	if (!ret || IsNull(~bodies.cy))
		bodies.cy <<= 0;
	if (!ret || IsNull(~bodies.cz))
		bodies.cz <<= 0;
	if (!ret || IsNull(~Nf))
		Nf <<= 100;
	if (!ret || IsNull(~minF))
		minF <<= 0;
	if (!ret || IsNull(~maxF))
		maxF <<= 4;
	if (!ret || IsNull(~Nh))
		Nh <<= 1;
	if (!ret || IsNull(~minH))
		minH <<= 0;
	if (!ret || IsNull(~maxH))
		maxH <<= 0;
	if (!ret || IsNull(~height)) {
		height <<= Null;
		opInfinite <<= true;
	}
}

void MainSolver::Load(const BEMData &bem) {
	if (IsNull(~nemoh.g))
		nemoh.g <<= bem.g;
	if (IsNull(~nemoh.rho))
		nemoh.rho <<= bem.rho;	
	if (IsNull(~height)) {
		opInfinite <<= (bem.depth < 0);
		height.Enable(bem.depth > 0);
		height <<= (bem.depth > 0 ? bem.depth : Null);
	}
}
	
void MainSolver::Jsonize(JsonIO &json) {
	if (json.IsLoading()) {
		opIncludeBin = Null;
		numSplit = Null;	
		nemoh.xeff <<= Null;
		nemoh.yeff <<= Null;
		bodies.cx <<= Null;
		bodies.cy <<= Null;
		bodies.cz <<= Null;
		Nf <<= Null;
		minF <<= Null;
		maxF <<= Null;
		Nh <<= Null;
		minH <<= Null;
		maxH <<= Null;
	} else {
		dropSolverVal = dropSolver.GetData();
	}
	json
		("loadFrom", loadFrom)
		("saveTo", saveTo)
		("opIncludeBin", opIncludeBin)
		("numSplit", numSplit)
		("numThreads", numThreads)
		("opSplit", opSplit)
		("xeff", nemoh.xeff)
		("yeff", nemoh.yeff)
		("cx", bodies.cx)
		("cy", bodies.cy)
		("cz", bodies.cz)
		("Nf", Nf)
		("minF", minF)
		("maxF", maxF)
		("Nh", Nh)
		("minH", minH)
		("maxH", maxH)
		("dropSolver", dropSolverVal)
		("height", height)
		("opInfinite", opInfinite)
	;
}

bool MainSolver::OnLoad() {
	String folder = ~loadFrom;
	
	try {
		String fileNemoh = AppendFileNameX(folder, "Nemoh.cal");
		if (!FileExists(fileNemoh))
			fileNemoh = folder;
		String fileHams = AppendFileNameX(folder, "Input", "ControlFile.in");
		if (!FileExists(fileHams)) {
			fileHams = AppendFileNameX(folder, "ControlFile.in");
			if (!FileExists(fileHams)) 
				fileHams = folder;
		}
				
		if (FileExists(fileNemoh)) {
			NemohCal data;			
			if (!data.Load(fileNemoh)) {
				Exclamation(Format("Problem loading %s file", DeQtf(fileNemoh)));
				return false;
			}
			Load(data);	
		} else if (FileExists(fileHams)) {
			HamsCal data;
			if (!data.Load(fileHams)) {
				Exclamation(Format("Problem loading %s file", DeQtf(fileHams)));
				return false;
			}
			Load(data);	
		}
		dropSolver.WhenAction();
		
	} catch (const Exc &e) {
		Exclamation(DeQtfLf(e));
		return false;
	}
	
	return true;
}

void MainSolver::InitArray(bool isNemoh) {
	bodies.array.Reset();
	bodies.array.SetLineCy(EditField::GetStdHeight());
	bodies.array.AddColumn("Mesh file", 40);
	bodies.array.AddColumn("Lid file", 40);
	bodies.array.AddColumn("#points", 10);
	bodies.array.AddColumn("#panels", 10);
	bodies.array.AddColumn("Surge", 10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("Sway",  10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("Heave", 10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("Roll",  10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("Pitch", 10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("Yaw",   10).With([](One<Ctrl>& x) {x.Create<Option>().SetReadOnly();});
	bodies.array.AddColumn("RotX",  10);
	bodies.array.AddColumn("RotY",  10);
	bodies.array.AddColumn("RotZ",  10);
	
	dropSolver.WhenAction();

	InitGrid(bodies.mass, editMass);
	InitGrid(bodies.linearDamping, editLinear);
	InitGrid(bodies.quadraticDamping, editQuadratic);	
	InitGrid(bodies.hydrostaticRestoring, editInternal);
	InitGrid(bodies.externalRestoring, editExternal);
}

void MainSolver::InitGrid(GridCtrl &grid, EditDouble edit[]) {
	grid.Reset();
	grid.Absolute().Editing().Clipboard();
	for (int i = 1; i <= 6; ++i)
		grid.AddColumn(FormatInt(i)).Edit(edit[i-1]);
	for (int y = 0; y < 6; ++y)
		for (int x = 0; x < 6; ++x)
			grid.Set(y, x, 0.);
}

void MainSolver::LoadMatrix(GridCtrl &grid, const Eigen::MatrixXd &mat) {
	for (int y = 0; y < 6; ++y)
		for (int x = 0; x < 6; ++x)
			grid.Set(y, x, mat(x, y));
}

void MainSolver::Load(const BemCal &data, bool isNemoh) {
	opInfinite <<= (data.h < 0);
	height.Enable(data.h > 0);
	height <<= (data.h > 0 ? data.h : Null);
	
	InitArray(true);
	
	for (int i = 0; i < data.bodies.size(); ++i) {
		const BemBody &b = data.bodies[i];
		bodies.array.Add(b.meshFile, b.lidFile, b.npoints, b.npanels, 
					b.dof[Hydro::SURGE], b.dof[Hydro::SWAY], b.dof[Hydro::HEAVE], 
					b.dof[Hydro::ROLL], b.dof[Hydro::PITCH], b.dof[Hydro::YAW], 
					b.c0[0], b.c0[1], b.c0[2]);
	}
	bodies.array.SetCursor(0);
	dropSolver.WhenAction();
		
	Nf <<= data.Nf;
	
	minF <<= data.minF;
	maxF <<= data.maxF;	
	
	Nh <<= data.Nh;
	minH <<= data.minH;
	maxH <<= data.maxH;
	
	if (isNemoh) 
		dropSolver <<= BemCal::NEMOHv115;
	else
		dropSolver <<= BemCal::HAMS;
}
	
void MainSolver::Load(const HamsCal &data) {
	Load(static_cast<const BemCal &>(data), false);

	const BemBody &b = data.bodies[0];
	
	bodies.xcm = b.cg[0];
	bodies.ycm = b.cg[1];
	bodies.zcm = b.cg[2];
	MatrixXdToGridCtrl(bodies.mass, b.mass);
	MatrixXdToGridCtrl(bodies.linearDamping, b.linearDamping);
	MatrixXdToGridCtrl(bodies.quadraticDamping, b.quadraticDamping);
	MatrixXdToGridCtrl(bodies.hydrostaticRestoring, b.hydrostaticRestoring);
	MatrixXdToGridCtrl(bodies.externalRestoring, b.externalRestoring);
}
	
void MainSolver::Load(const NemohCal &data) {
	Load(static_cast<const BemCal &>(data), true);
	
	nemoh.g <<= data.g;
	nemoh.rho <<= data.rho;
	
	nemoh.xeff <<= data.xeff;
	nemoh.yeff <<= data.yeff;
	
	nemoh.boxIrf <<= data.irf;
	nemoh.irfStep <<= data.irfStep;
	nemoh.irfDuration <<= data.irfDuration;
	
	nemoh.boxKochin <<= (data.nKochin > 0);
	nemoh.nKochin <<= data.nKochin;
	nemoh.minK <<= data.minK;
	nemoh.maxK <<= data.maxK;
	
	nemoh.showPressure <<= data.showPressure;
	
	nemoh.freeSurface <<= (data.nFreeX > 0);
	
	nemoh.freeX <<= (data.nFreeX > 0);
	nemoh.nFreeX <<= data.nFreeX;
	nemoh.domainX <<= data.domainX;
		
	nemoh.freeY <<= (data.nFreeY > 0);
	nemoh.nFreeY <<= data.nFreeY;
	nemoh.domainY <<= data.domainY;
}

bool MainSolver::Save(BemCal &data, bool isNemoh) {
	if (!opInfinite)
		data.h = ~height;
	else
		data.h = -1;
	
	data.bodies.SetCount(bodies.array.GetCount());
	for (int i = 0; i < data.bodies.size(); ++i) {
		BemBody &b = data.bodies[i];
		b.meshFile = bodies.array.Get(i, 0);
		if (isNemoh) {
			if (FileExists(b.meshFile)) {
				MeshData dat;
				bool y0z, x0z;
				String ret = dat.Load(b.meshFile, y0z, x0z);
				int factor = (y0z ? 2 : 1)*(x0z ? 2 : 1);
				if (ret.IsEmpty()) {
					b.npoints = dat.mesh.GetNumNodes()/factor;
					b.npanels = dat.mesh.GetNumPanels()/factor;
				} else 
					return false;
			}
		}
		b.lidFile = bodies.array.Get(i, 1);

		b.dof[Hydro::SURGE] = bodies.array.Get(i, 4);
		b.dof[Hydro::SWAY]  = bodies.array.Get(i, 5);
		b.dof[Hydro::HEAVE] = bodies.array.Get(i, 6);
		b.dof[Hydro::ROLL]  = bodies.array.Get(i, 7);
		b.dof[Hydro::PITCH] = bodies.array.Get(i, 8);
		b.dof[Hydro::YAW]   = bodies.array.Get(i, 9);
		b.c0[0] 	 = bodies.array.Get(i, 10);
		b.c0[1] 	 = bodies.array.Get(i, 11);
		b.c0[2] 	 = bodies.array.Get(i, 12);
	}
		
	data.Nf = ~Nf;
	
	data.minF = ~minF;
	data.maxF = ~maxF;	
	
	data.Nh = ~Nh;
	data.minH = ~minH;
	data.maxH = ~maxH;
	
	return true;
}

bool MainSolver::Save(HamsCal &data) {
	Save(static_cast<BemCal &>(data), false);
	
	BemBody &b = data.bodies[0];
	
	b.cg[0] = bodies.cx;
	b.cg[1] = bodies.cy;
	b.cg[2] = bodies.cz;
	GridCtrlToMatrixXd(b.mass, bodies.mass);
	GridCtrlToMatrixXd(b.linearDamping, bodies.linearDamping);
	GridCtrlToMatrixXd(b.quadraticDamping, bodies.quadraticDamping);
	GridCtrlToMatrixXd(b.hydrostaticRestoring, bodies.hydrostaticRestoring);
	GridCtrlToMatrixXd(b.externalRestoring, bodies.externalRestoring);
	
	return true;
}

bool MainSolver::Save(NemohCal &data) {
	Save(static_cast<BemCal &>(data), true);
	
	data.g = ~nemoh.g;
	data.rho = ~nemoh.rho;
	
	data.xeff = ~nemoh.xeff;
	data.yeff = ~nemoh.yeff;
	
	data.irf = ~nemoh.boxIrf;
	if (~nemoh.boxIrf) {
		data.irfStep = ~nemoh.irfStep;
		data.irfDuration = ~nemoh.irfDuration;
	} else
		data.irfStep = data.irfDuration = 0;
	
	if (~nemoh.boxKochin) {
		data.nKochin = ~nemoh.nKochin;
		data.minK = ~nemoh.minK;
		data.maxK = ~nemoh.maxK;
	} else {
		data.nKochin = 0;
		data.minK = data.maxK = 0;
	}
	
	data.showPressure = ~nemoh.showPressure;
	
	if (~nemoh.freeSurface) {
		data.nFreeX = ~nemoh.nFreeX;
		data.domainX = ~nemoh.domainX;
		
		data.nFreeY = ~nemoh.nFreeY;
		data.domainY = ~nemoh.domainY;	
	} else {
		data.nFreeX = data.nFreeY = 0;
		data.domainX = data.domainY = 0;
	}
	return true;
}

void MainSolver::arrayOnCursor() {
	int id = bodies.array.GetCursor();
	if (id < 0)
		return;
	
	bodies.meshFile <<= bodies.array.Get(id, 0);
	bodies.lidFile  <<= bodies.array.Get(id, 1);
	bodies.surge 	<<= bodies.array.Get(id, 4);
	bodies.sway 	<<= bodies.array.Get(id, 5);
	bodies.heave 	<<= bodies.array.Get(id, 6);
	bodies.roll 	<<= bodies.array.Get(id, 7);
	bodies.pitch 	<<= bodies.array.Get(id, 8);
	bodies.yaw 	 	<<= bodies.array.Get(id, 9);
	bodies.cx 		<<= bodies.array.Get(id, 10);
	bodies.cy 		<<= bodies.array.Get(id, 11);
	bodies.cz 		<<= bodies.array.Get(id, 12);
}

bool MainSolver::ArrayUpdateCursor() {
	try {
		bool isNemoh = ~dropSolver != BemCal::HAMS;
		
		int id = bodies.array.GetCursor();
		if (id < 0) {
			if (bodies.array.GetCount() == 0) {
				InitArray(isNemoh);
				bodies.array.Add();
				id = 0;
				bodies.cx <<= 0;
				bodies.cy <<= 0;
				bodies.cz <<= 0;
			} else
				id = bodies.array.GetCount()-1;
		}	
		
		if (isNemoh && ~bodies.meshFile != bodies.array.Get(id, 0)) {
			MeshData dat;
			bool y0z, x0z;
			String ret = dat.Load(~bodies.meshFile, y0z, x0z);
			int factor = (y0z ? 2 : 1)*(x0z ? 2 : 1);
			if (ret.IsEmpty()) {
				bodies.array.Set(id, 2, dat.mesh.GetNumNodes()/factor);
				bodies.array.Set(id, 3, dat.mesh.GetNumPanels()/factor);
			} /*else {
				Exclamation(DeQtf(ret));
				return false;		
			}*/
		} else {
			bodies.array.Set(id, 2, "-");
			bodies.array.Set(id, 3, "-");
		}
		bodies.array.Set(id, 0, ~bodies.meshFile);
		bodies.array.Set(id, 1, ~bodies.lidFile);
		
		bodies.array.Set(id, 4, ~bodies.surge);
		bodies.array.Set(id, 5, ~bodies.sway);
		bodies.array.Set(id, 6, ~bodies.heave);
		bodies.array.Set(id, 7, ~bodies.roll);
		bodies.array.Set(id, 8, ~bodies.pitch);
		bodies.array.Set(id, 9, ~bodies.yaw);
		bodies.array.Set(id, 10,~bodies.cx);
		bodies.array.Set(id, 11,~bodies.cy);
		bodies.array.Set(id, 12,~bodies.cz);
	
		bodies.array.Update();
		
	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
		return false;
	}
	
	return true;
}

void MainSolver::arrayClear() {
	bodies.meshFile <<= "";
	bodies.lidFile  <<= "";
	bodies.surge 	<<= false;
	bodies.sway 	<<= false;
	bodies.heave 	<<= false;
	bodies.roll 	<<= false;
	bodies.pitch 	<<= false;
	bodies.yaw 	 	<<= false;
	bodies.cx 		<<= 0;
	bodies.cy 		<<= 0;
	bodies.cz 		<<= 0;
}

void MainSolver::arrayOnAdd() {
	if (bodies.array.GetCount() == 0) {
		bool isNemoh = ~dropSolver != BemCal::HAMS;
		InitArray(isNemoh);
	}
	bodies.array.Add();
	bodies.array.SetCursor(bodies.array.GetCount()-1);	
	arrayClear();
	ArrayUpdateCursor();
	dropSolver.WhenAction();
}

void MainSolver::arrayOnDuplicate() {
	if (bodies.array.GetCount() == 0) {
		Exclamation(t_("No body available to duplicate"));
		return;
	}
	int id = bodies.array.GetCursor();
	if (id < 0) {
		Exclamation(t_("Please select body to duplicate"));
		return;
	}
	int nr = id + 1;
	bodies.array.Insert(nr);
	for (int c = 0; c < bodies.array.GetColumnCount(); ++c)
		bodies.array.Set(nr, c, bodies.array.Get(id, c));
	bodies.array.Disable();
	bodies.array.SetCursor(nr);
	bodies.array.Enable();
	arrayOnCursor();
}

void MainSolver::arrayOnRemove() {
	if (bodies.array.GetCount() == 0) {
		Exclamation(t_("No body available to remove"));
		return;
	}
	int id = bodies.array.GetCursor();
	if (id < 0) {
		Exclamation(t_("Please select body to remove"));
		return;
	}
	bodies.array.Remove(id);
	if (id >= bodies.array.GetCount()) {
		id = bodies.array.GetCount()-1;
		if (id < 0) {
			arrayClear();
			ArrayUpdateCursor();
			return;
		}
	} 
	bodies.array.SetCursor(id);
}

bool MainSolver::OnSave(const BEMData &bem) {
	try {
		String folder = ~saveTo;
		
		bool isNemoh = ~dropSolver != BemCal::HAMS;
		
		NemohCal dataNemoh;
		HamsCal dataHams;
		
		auto lambda = [&]() -> BemCal & {
			if (isNemoh)
				return dataNemoh;
			else
				return dataHams;
		};
		BemCal &data = lambda();
		
		Upp::Vector<String> errors;
		
		if (isNemoh) {
			if (!Save(dataNemoh))
				return false;
			errors = dataNemoh.Check();
		} else {
			if (!Save(dataHams))
				return false;
			errors = dataHams.Check();
		}
		
		if (!errors.IsEmpty()) {
			String str;
			for (int i = 0; i < errors.size(); ++i)
			 	str << "\n- " << errors[i];
			if (!ErrorOKCancel(Format(t_("Errors found in data:%s&Do you wish to continue?"), DeQtfLf(str))))
				return false;
		}
		if (!DirectoryExists(folder)) {
			if (!PromptYesNo(Format(t_("Folder %s does not exist.&Do you wish to create it?"), DeQtfLf(folder))))
				return false;
			RealizeDirectory(folder);
		} else {
			if (!PromptYesNo(Format(t_("Folder %s contents will be deleted.&Do you wish to continue?"), DeQtfLf(folder))))
				return false;
		}
		if (~opSplit) {
			if (IsNull(~numSplit)) {
				Exclamation(t_("Please enter number of parts to split the simulation (min. is 2)"));
				return false;
			} else if (int(~numSplit) > data.Nf) {
				if (PromptOKCancel(Format(t_("Number of split cases %d must not be higher than number of frequencies %d"), int(~numSplit), data.Nf)
							   + S("&") + t_("Do you wish to fit the number of cases to the number of frequencies?"))) 
					numSplit <<= data.Nf;
				else
					return false;
			}
		}
		if (isNemoh) 
			dataNemoh.SaveFolder(folder, ~opIncludeBin, ~opSplit ? int(~numSplit) : 1, bem, ~dropSolver);
		else
			dataHams.SaveFolder(folder, ~opIncludeBin, ~opSplit ? int(~numSplit) : 1, ~numThreads, bem);

	} catch (Exc e) {
		Exclamation(DeQtfLf(e));
		return false;
	}
	
	return true;
}

void GridCtrlToMatrixXd(Eigen::MatrixXd &mat, const GridCtrl &grid) {
	mat.resize(grid.GetColumnCount(), grid.GetRowCount());
	
	for (int x = 0; x < mat.cols(); ++x)
		for (int y = 0; y < mat.cols(); ++y)
			mat(x, y) = grid.Get(y, x);
}

void MatrixXdToGridCtrl(GridCtrl &grid, const Eigen::MatrixXd &mat) {
	for (int x = 0; x < mat.cols(); ++x)
		for (int y = 0; y < mat.cols(); ++y)
			grid.Set(y, x, mat(x, y));
}
