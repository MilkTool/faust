/************************************************************************
 ************************************************************************
    FAUST compiler
    Copyright (C) 2003-2015 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/
 
#ifndef interpreter_dsp_aux_h
#define interpreter_dsp_aux_h

#include "faust/dsp/dsp.h"
#include "faust/gui/UI.h"
#include "faust/gui/meta.h"
#include "fir_interpreter.hh"
//#include "smartpointer.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__ ((visibility("default")))
#endif

class interpreter_dsp;

template <class T>
class interpreter_dsp_aux;

struct EXPORT interpreter_dsp_factory : public dsp_factory {
    
    std::string fExpandedDSP;
    std::string fShaKey;
    std::string fName;
    
    float fVersion;
    int fNumInputs;
    int fNumOutputs;
    
    int fIntHeapSize;
    int fRealHeapSize;
    int fSROffset;
    int fCountOffset;
    
    FIRMetaBlockInstruction* fMetaBlock;
    FIRUserInterfaceBlockInstruction<float>* fUserInterfaceBlock;
    FIRBlockInstruction<float>* fInitBlock;
    FIRBlockInstruction<float>* fComputeBlock;
    FIRBlockInstruction<float>* fComputeDSPBlock;
    
    interpreter_dsp_factory(const std::string& name,
                            float version_num,
                            int inputs, int ouputs,
                            int int_heap_size, int real_heap_size,
                            int sr_offset, int count_offset,
                            FIRMetaBlockInstruction* meta,
                            FIRUserInterfaceBlockInstruction<float>* interface,
                            FIRBlockInstruction<float>* init,
                            FIRBlockInstruction<float>* compute_control,
                            FIRBlockInstruction<float>* compute_dsp)
    :fName(name),
    fVersion(version_num),
    fNumInputs(inputs),
    fNumOutputs(ouputs),
    fIntHeapSize(int_heap_size),
    fRealHeapSize(real_heap_size),
    fSROffset(sr_offset),
    fCountOffset(count_offset),
    fMetaBlock(meta),
    fUserInterfaceBlock(interface),
    fInitBlock(init),
    fComputeBlock(compute_control),
    fComputeDSPBlock(compute_dsp)
    {}
    
    virtual ~interpreter_dsp_factory()
    {
        // No more DSP instances, so delete
        delete fUserInterfaceBlock;
        delete fInitBlock;
        delete fComputeBlock;
        delete fComputeDSPBlock;
    }
    
    /* Return Factory name */
    EXPORT std::string getName();
    
    /* Return Factory SHA key */
    EXPORT std::string getSHAKey();
    
    /* Return Factory expanded DSP code */
    EXPORT std::string getDSPCode();
    
    EXPORT dsp* createDSPInstance();
    
    EXPORT void metadata(Meta* meta);
    
    void write(std::ostream* out);
    
    static interpreter_dsp_factory* read(std::istream* in);
    
    static FIRMetaBlockInstruction* readMetaBlock(std::istream* in);
    
    static FIRMetaInstruction* readMetaInstruction(std::stringstream* inst);
    
    static FIRUserInterfaceBlockInstruction<float>* readUIBlock(std::istream* in);
    
    static FIRUserInterfaceInstruction<float>* readUIInstruction(std::stringstream* inst);
    
    static FIRBlockInstruction<float>* readCodeBlock(std::istream* in);
    
    static FIRBasicInstruction<float>* readCodeInstruction(std::istream* inst, std::istream* in);
    
    void ExecuteMeta(FIRMetaBlockInstruction* block, Meta* meta)
    {
        MetaInstructionIT it;
        
        for (it = block->fInstructions.begin(); it != block->fInstructions.end(); it++) {
            meta->declare((*it)->fKey.c_str(), (*it)->fValue.c_str());
        }
    }
    
};

template <class T>
class interpreter_dsp_aux : public dsp, public FIRInterpreter<T> {
	
    protected:
    
        interpreter_dsp_factory* fFactory;
   	
    public:
    
        interpreter_dsp_aux(interpreter_dsp_factory* factory)
        : FIRInterpreter<T>(factory->fIntHeapSize, factory->fRealHeapSize, factory->fSROffset, factory->fCountOffset)
        {
            fFactory = factory;
            
            this->fInputs = new FAUSTFLOAT*[fFactory->fNumInputs];
            this->fOutputs = new FAUSTFLOAT*[fFactory->fNumOutputs];
        }
    
        virtual ~interpreter_dsp_aux()
        {
            delete [] this->fInputs;
            delete [] this->fOutputs;
        }
          
        virtual int getNumInputs()
        {
            return this->fFactory->fNumInputs;
        }
        
        virtual int getNumOutputs() 
        {
            return this->fFactory->fNumOutputs;
        }
        
        virtual int getInputRate(int channel) 
        {
            return -1;
        }
        
        virtual int getOutputRate(int channel) 
        {
            return -1;
        }
        
        static void classInit(int samplingRate)
        {}
        
        virtual void instanceInit(int samplingRate)
        {
            // Store samplingRate in 'fSamplingFreq' variable at correct offset in fIntHeap
            this->fIntHeap[this->fSROffset] = samplingRate;
            
            // Execute init instructions
            this->ExecuteBlock(fFactory->fInitBlock);
        }
    
        virtual void init(int samplingRate)
        {
            classInit(samplingRate);
            this->instanceInit(samplingRate);
        }
    
        virtual void buildUserInterface(UI* interface)
        {
            this->ExecuteBuildUserInterface(fFactory->fUserInterfaceBlock, interface);
        }
    
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
        {
            // Prepare in/out buffers
            for (int i = 0; i < this->fFactory->fNumInputs; i++) {
                this->fInputs[i] = inputs[i];
            }
            for (int i = 0; i < this->fFactory->fNumOutputs; i++) {
                this->fOutputs[i] = outputs[i];
            }
            
            // Executes the 'control' block
            this->ExecuteBlock(this->fFactory->fComputeBlock);
            
            // Set count in 'count' variable at the correct offset in fIntHeap
            this->fIntHeap[this->fCountOffset] = count;
            
            // Executes the 'DSP' block
            this->ExecuteBlock(this->fFactory->fComputeDSPBlock);
            
             //std::cout << "sample " << outputs[0][0] << std::endl;
        }
    
};

/*
Computing on a downsampled version of signals
 
TODO:
 
- anti alias filter at input
 
- interpolation at output
 
*/

template <class T>
class interpreter_dsp_aux_down : public interpreter_dsp_aux<T> {
    
    private:
    
        int fDownSamplingFactor;

    public:
    
        interpreter_dsp_aux_down(interpreter_dsp_factory* factory, int down_sampling_factor)
            : interpreter_dsp_aux<T>(factory), fDownSamplingFactor(down_sampling_factor)
        {
            // Allocate and set downsampled inputs/outputs
            for (int i = 0; i < this->fFactory->fNumInputs; i++) {
                this->fInputs[i] = new T[2048];
            }
            for (int i = 0; i < this->fFactory->fNumOutputs; i++) {
                this->fOutputs[i] = new T[2048];
            }
        }
        
        virtual ~interpreter_dsp_aux_down()
        {
            // Delete downsampled inputs/outputs
            for (int i = 0; i < this->fFactory->fNumInputs; i++) {
                delete [] this->fInputs[i];
            }
            for (int i = 0; i < this->fFactory->fNumOutputs; i++) {
                delete [] this->fOutputs[i];
            }
        }
    
        static void classInit(int samplingRate)
        {}

        virtual void init(int samplingRate)
        {
            classInit(samplingRate / fDownSamplingFactor);
            this->instanceInit(samplingRate / fDownSamplingFactor);
        }
    
        virtual void compute(int count, FAUSTFLOAT** inputs, FAUSTFLOAT** outputs)
        {
            // Downsample inputs
            for (int i = 0; i < this->fFactory->fNumInputs; i++) {
                for (int j = 0; j < count / fDownSamplingFactor; j++) {
                    this->fInputs[i][j] = inputs[i][j * fDownSamplingFactor];
                }
            }
            
            // Executes the 'control' block
            this->ExecuteBlock(this->fFactory->fComputeBlock);
            
            // Set count in 'count' variable at the correct offset in fIntHeap
            this->fIntHeap[this->fCountOffset] = count / fDownSamplingFactor;
            
            // Executes the 'DSP' block
            this->ExecuteBlock(this->fFactory->fComputeDSPBlock);
            
            // Upsample ouputs
            for (int i = 0; i < this->fFactory->fNumOutputs; i++) {
                for (int j = 0; j < count / fDownSamplingFactor; j++) {
                    T sample = this->fOutputs[i][j];
                    outputs[i][j * fDownSamplingFactor] = sample;
                    outputs[i][(j * fDownSamplingFactor) + 1] = sample;
                }
            }
            
            //std::cout << "sample " << outputs[0][0] << std::endl;
        }
    
};

class EXPORT interpreter_dsp : public dsp {
    
    public:
    
        int getNumInputs();
        int getNumOutputs();
    
        void init(int samplingRate);
        void instanceInit(int samplingRate);
      
        void buildUserInterface(UI* ui_interface);
        
        void compute(int count, FAUSTFLOAT** input, FAUSTFLOAT** output);
    
};

// Public C++ interface

EXPORT interpreter_dsp_factory* getInterpreterDSPFactoryFromSHAKey(const std::string& sha_key);

EXPORT interpreter_dsp_factory* createInterpreterDSPFactoryFromFile(const std::string& filename, 
                                                                  int argc, const char* argv[], 
                                                                  std::string& error_msg);

EXPORT interpreter_dsp_factory* createInterpreterDSPFactoryFromString(const std::string& name_app,
                                                                    const std::string& dsp_content,
                                                                    int argc, const char* argv[], 
                                                                    std::string& error_msg);

EXPORT bool deleteInterpreterDSPFactory(interpreter_dsp_factory* factory);

EXPORT std::vector<std::string> getInterpreterDSPFactoryLibraryList(interpreter_dsp_factory* factory);

EXPORT std::vector<std::string> getAllInterpreterDSPFactories();

EXPORT interpreter_dsp_factory* readInterpreterDSPFactoryFromMachine(const std::string& machine_code);

EXPORT std::string writeInterpreterDSPFactoryToMachine(interpreter_dsp_factory* factory);

EXPORT interpreter_dsp_factory* readInterpreterDSPFactoryFromMachineFile(const std::string& machine_code_path);

EXPORT void writeInterpreterDSPFactoryToMachineFile(interpreter_dsp_factory* factory, const std::string& machine_code_path);

EXPORT void deleteAllInterpreterDSPFactories();

EXPORT interpreter_dsp* createInterpreterDSPInstance(interpreter_dsp_factory* factory);

EXPORT void deleteInterpreterDSPInstance(interpreter_dsp* dsp);

#endif
