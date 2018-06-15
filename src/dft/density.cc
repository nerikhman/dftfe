// ---------------------------------------------------------------------
//
// Copyright (c) 2017-2018 The Regents of the University of Michigan and DFT-FE authors.
//
// This file is part of the DFT-FE code.
//
// The DFT-FE code is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE at
// the top level of the DFT-FE distribution.
//
// ---------------------------------------------------------------------
//
// @author Shiva Rudraraju, Phani Motamarri, Krishnendu Ghosh, Sambit Das
//

//source file for electron density related computations

//calculate electron density
template<unsigned int FEOrder>
void dftClass<FEOrder>::compute_rhoOut()
{
  const unsigned int numEigenVectors=numEigenValues;
  const unsigned int numKPoints=d_kPointWeights.size();

#ifdef USE_COMPLEX
  FEEvaluation<3,FEOrder,C_num1DQuad<FEOrder>(),2> psiEval(matrix_free_data,eigenDofHandlerIndex , 0);
#else
  FEEvaluation<3,FEOrder,C_num1DQuad<FEOrder>(),1> psiEval(matrix_free_data,eigenDofHandlerIndex , 0);
#endif
  const unsigned int numQuadPoints=psiEval.n_q_points;

  Tensor<1,2,VectorizedArray<double> > zeroTensor1;
  zeroTensor1[0]=make_vectorized_array(0.0);
  zeroTensor1[1]=make_vectorized_array(0.0);
  Tensor<1,2, Tensor<1,3,VectorizedArray<double> > > zeroTensor2;
  Tensor<1,3,VectorizedArray<double> > zeroTensor3;
  for (unsigned int idim=0; idim<3; idim++)
  {
    zeroTensor2[0][idim]=make_vectorized_array(0.0);
    zeroTensor2[1][idim]=make_vectorized_array(0.0);
    zeroTensor3[idim]=make_vectorized_array(0.0);
  }

  //create new rhoValue tables
  rhoOutVals.push_back(std::map<dealii::CellId,std::vector<double> > ());
  rhoOutValues = &(rhoOutVals.back());
  if (dftParameters::spinPolarized==1)
    {
    rhoOutValsSpinPolarized.push_back(std::map<dealii::CellId,std::vector<double> > ());
    rhoOutValuesSpinPolarized = &(rhoOutValsSpinPolarized.back());
    }

  if(dftParameters::xc_id == 4)
    {
      gradRhoOutVals.push_back(std::map<dealii::CellId, std::vector<double> >());
      gradRhoOutValues = &(gradRhoOutVals.back());
      if (dftParameters::spinPolarized==1)
       {
         gradRhoOutValsSpinPolarized.push_back(std::map<dealii::CellId, std::vector<double> >());
         gradRhoOutValuesSpinPolarized = &(gradRhoOutValsSpinPolarized.back());
       }
    }

   for (unsigned int cell=0; cell<matrix_free_data.n_macro_cells(); ++cell)
   {
	  const unsigned int numSubCells=matrix_free_data.n_components_filled(cell);

	  for (unsigned int iSubCell=0; iSubCell<numSubCells; ++iSubCell)
	  {
		const dealii::CellId subCellId=matrix_free_data.get_cell_iterator(cell,iSubCell)->id();
		(*rhoOutValues)[subCellId] = std::vector<double>(numQuadPoints,0.0);
		if(dftParameters::xc_id == 4)
		    (*gradRhoOutValues)[subCellId] = std::vector<double>(3*numQuadPoints,0.0);

		if (dftParameters::spinPolarized==1)
		{
		    (*rhoOutValuesSpinPolarized)[subCellId] = std::vector<double>(2*numQuadPoints,0.0);
		     if(dftParameters::xc_id == 4)
		        (*gradRhoOutValuesSpinPolarized)[subCellId]
			 = std::vector<double>(6*numQuadPoints,0.0);
		}
	  }
   }

   //temp arrays
   std::vector<double> rhoTemp(numQuadPoints), rhoTempSpinPolarized(2*numQuadPoints), rhoOut(numQuadPoints), rhoOutSpinPolarized(2*numQuadPoints);
   std::vector<double> gradRhoTemp(3*numQuadPoints), gradRhoTempSpinPolarized(6*numQuadPoints),gradRhoOut(3*numQuadPoints), gradRhoOutSpinPolarized(6*numQuadPoints);


   const unsigned int numBlockEigenVec=1;
   std::vector<vectorType> eigenVectors((1+dftParameters::spinPolarized)*d_kPointWeights.size());
   for(unsigned int ivec = 0; ivec < numEigenValues; ++ivec)
   {
      for(unsigned int kPoint = 0; kPoint < (1+dftParameters::spinPolarized)*d_kPointWeights.size(); ++kPoint)
      {

          eigenVectors[kPoint].reinit(d_tempEigenVec);
#ifdef USE_COMPLEX
	    vectorTools::copyFlattenedDealiiVecToSingleCompVec
		     (d_eigenVectorsFlattened[kPoint],
		      numEigenValues,
		      ivec,
		      localProc_dof_indicesReal,
		      localProc_dof_indicesImag,
		      eigenVectors[kPoint]);
#else
	    vectorTools::copyFlattenedDealiiVecToSingleCompVec
		     (d_eigenVectorsFlattened[kPoint],
		      numEigenValues,
		      ivec,
		      eigenVectors[kPoint]);
#endif
      }

      for (unsigned int cell=0; cell<matrix_free_data.n_macro_cells(); ++cell)
      {
	      psiEval.reinit(cell);

	      const unsigned int numSubCells=matrix_free_data.n_components_filled(cell);

#ifdef USE_COMPLEX
	      std::vector<Tensor<1,2,VectorizedArray<double> > > psiQuads(numQuadPoints*numBlockEigenVec*numKPoints,zeroTensor1);
	      std::vector<Tensor<1,2,VectorizedArray<double> > > psiQuads2(numQuadPoints*numBlockEigenVec*numKPoints,zeroTensor1);
	      std::vector<Tensor<1,2,Tensor<1,3,VectorizedArray<double> > > > gradPsiQuads(numQuadPoints*numBlockEigenVec*numKPoints,zeroTensor2);
	      std::vector<Tensor<1,2,Tensor<1,3,VectorizedArray<double> > > > gradPsiQuads2(numQuadPoints*numBlockEigenVec*numKPoints,zeroTensor2);
#else
	      std::vector< VectorizedArray<double> > psiQuads(numQuadPoints*numBlockEigenVec,make_vectorized_array(0.0));
	      std::vector< VectorizedArray<double> > psiQuads2(numQuadPoints*numBlockEigenVec,make_vectorized_array(0.0));
	      std::vector<Tensor<1,3,VectorizedArray<double> > > gradPsiQuads(numQuadPoints*numBlockEigenVec,zeroTensor3);
	      std::vector<Tensor<1,3,VectorizedArray<double> > > gradPsiQuads2(numQuadPoints*numBlockEigenVec,zeroTensor3);
#endif

	      for(unsigned int kPoint = 0; kPoint < numKPoints; ++kPoint)
		  for(unsigned int iEigenVec=0; iEigenVec<numBlockEigenVec; ++iEigenVec)
		    {

		       psiEval.read_dof_values_plain(eigenVectors[(1+dftParameters::spinPolarized)*kPoint]);

		       if(dftParameters::xc_id == 4)
			  psiEval.evaluate(true,true);
		       else
			  psiEval.evaluate(true,false);

		       for (unsigned int q=0; q<numQuadPoints; ++q)
		       {
			 psiQuads[q*numBlockEigenVec*numKPoints+numBlockEigenVec*kPoint+iEigenVec]=psiEval.get_value(q);
			 if(dftParameters::xc_id == 4)
			    gradPsiQuads[q*numBlockEigenVec*numKPoints+numBlockEigenVec*kPoint+iEigenVec]=psiEval.get_gradient(q);
		       }

		       if(dftParameters::spinPolarized==1)
		       {

			   psiEval.read_dof_values_plain(eigenVectors[(1+dftParameters::spinPolarized)*kPoint+1]);

			   if(dftParameters::xc_id == 4)
			      psiEval.evaluate(true,true);
			   else
			      psiEval.evaluate(true,false);

			   for (unsigned int q=0; q<numQuadPoints; ++q)
			   {
			     psiQuads2[q*numBlockEigenVec*numKPoints+numBlockEigenVec*kPoint+iEigenVec]=psiEval.get_value(q);
			     if(dftParameters::xc_id == 4)
				gradPsiQuads2[q*numBlockEigenVec*numKPoints+numBlockEigenVec*kPoint+iEigenVec]=psiEval.get_gradient(q);
			   }
		       }
		    }//eigenvector per k point

	      for (unsigned int iSubCell=0; iSubCell<numSubCells; ++iSubCell)
	      {
		    const dealii::CellId subCellId=matrix_free_data.get_cell_iterator(cell,iSubCell)->id();

		    std::fill(rhoTemp.begin(),rhoTemp.end(),0.0); std::fill(rhoOut.begin(),rhoOut.end(),0.0);

		    if (dftParameters::spinPolarized==1)
			std::fill(rhoTempSpinPolarized.begin(),rhoTempSpinPolarized.end(),0.0);

		    if(dftParameters::xc_id == 4)
		    {
		      std::fill(gradRhoTemp.begin(),gradRhoTemp.end(),0.0);
		      if (dftParameters::spinPolarized==1)
			  std::fill(gradRhoTempSpinPolarized.begin(),gradRhoTempSpinPolarized.end(),0.0);
		    }

		    for(unsigned int kPoint = 0; kPoint < numKPoints; ++kPoint)
		      for(unsigned int iEigenVec=0; iEigenVec<numBlockEigenVec; ++iEigenVec)
			{

			  const double partialOccupancy=dftUtils::getPartialOccupancy
							(eigenValues[kPoint][ivec+iEigenVec],
							 fermiEnergy,
							 C_kb,
							 dftParameters::TVal);

			  const double partialOccupancy2=dftUtils::getPartialOccupancy
							(eigenValues[kPoint][ivec+iEigenVec+dftParameters::spinPolarized*numEigenVectors],
							 fermiEnergy,
							 C_kb,
							 dftParameters::TVal);

			  for(unsigned int q=0; q<numQuadPoints; ++q)
			    {
			      const unsigned int id=q*numBlockEigenVec*numKPoints+numBlockEigenVec*kPoint+iEigenVec;
#ifdef USE_COMPLEX
			      Vector<double> psi, psi2;
			      psi.reinit(2); psi2.reinit(2);

			      psi(0)= psiQuads[id][0][iSubCell];
			      psi(1)=psiQuads[id][1][iSubCell];

			      if(dftParameters::spinPolarized==1)
			      {
				psi2(0)=psiQuads2[id][0][iSubCell];
				psi2(1)=psiQuads2[id][1][iSubCell];
			      }

			      std::vector<Tensor<1,3,double> > gradPsi(2),gradPsi2(2);

			      if(dftParameters::xc_id == 4)
				  for(unsigned int idim=0; idim<3; ++idim)
				  {
				     gradPsi[0][idim]=gradPsiQuads[id][0][idim][iSubCell];
				     gradPsi[1][idim]=gradPsiQuads[id][1][idim][iSubCell];

				     if(dftParameters::spinPolarized==1)
				     {
					 gradPsi2[0][idim]=gradPsiQuads2[id][0][idim][iSubCell];
					 gradPsi2[1][idim]=gradPsiQuads2[id][1][idim][iSubCell];
				     }
				  }
#else
			      double psi, psi2;
			      psi=psiQuads[id][iSubCell];
			      if (dftParameters::spinPolarized==1)
				  psi2=psiQuads2[id][iSubCell];

			      Tensor<1,3,double> gradPsi,gradPsi2;
			      if(dftParameters::xc_id == 4)
				  for(unsigned int idim=0; idim<3; ++idim)
				  {
				     gradPsi[idim]=gradPsiQuads[id][idim][iSubCell];
				     if(dftParameters::spinPolarized==1)
					 gradPsi2[idim]=gradPsiQuads2[id][idim][iSubCell];
				  }

#endif

#ifdef USE_COMPLEX
			      if(dftParameters::spinPolarized==1)
				{
				  rhoTempSpinPolarized[2*q] += partialOccupancy*d_kPointWeights[kPoint]*(psi(0)*psi(0) + psi(1)*psi(1));
				  rhoTempSpinPolarized[2*q+1] += partialOccupancy2*d_kPointWeights[kPoint]*(psi2(0)*psi2(0) + psi2(1)*psi2(1));
				  //
				  if(dftParameters::xc_id == 4)
				      for(unsigned int idim=0; idim<3; ++idim)
				      {
					  gradRhoTempSpinPolarized[6*q + idim] +=
					  2.0*partialOccupancy*d_kPointWeights[kPoint]*(psi(0)*gradPsi[0][idim] + psi(1)*gradPsi[1][idim]);
					  gradRhoTempSpinPolarized[6*q + 3+idim] +=
					  2.0*partialOccupancy2*d_kPointWeights[kPoint]*(psi2(0)*gradPsi2[0][idim] + psi2(1)*gradPsi2[1][idim]);
				      }
				}
			      else
				{
				  rhoTemp[q] += 2.0*partialOccupancy*d_kPointWeights[kPoint]*(psi(0)*psi(0) + psi(1)*psi(1));
				  if(dftParameters::xc_id == 4)
				    for(unsigned int idim=0; idim<3; ++idim)
				       gradRhoTemp[3*q + idim] += 2.0*2.0*partialOccupancy*d_kPointWeights[kPoint]*(psi(0)*gradPsi[0][idim] + psi(1)*gradPsi[1][idim]);
				}
#else
			      if(dftParameters::spinPolarized==1)
				{
				  rhoTempSpinPolarized[2*q] += partialOccupancy*psi*psi;
				  rhoTempSpinPolarized[2*q+1] += partialOccupancy2*psi2*psi2;

				  if(dftParameters::xc_id == 4)
				      for(unsigned int idim=0; idim<3; ++idim)
				      {
					  gradRhoTempSpinPolarized[6*q + idim] += 2.0*partialOccupancy*(psi*gradPsi[idim]);
					  gradRhoTempSpinPolarized[6*q + 3+idim] +=  2.0*partialOccupancy2*(psi2*gradPsi2[idim]);
				      }
				}
			      else
				{
				  rhoTemp[q] += 2.0*partialOccupancy*psi*psi;

				  if(dftParameters::xc_id == 4)
				    for(unsigned int idim=0; idim<3; ++idim)
				       gradRhoTemp[3*q + idim] += 2.0*2.0*partialOccupancy*psi*gradPsi[idim];
				}

#endif
			    }//quad point loop
			}//block eigenvectors per k point

		    for (unsigned int q=0; q<numQuadPoints; ++q)
		    {
			if(dftParameters::spinPolarized==1)
			{
				(*rhoOutValuesSpinPolarized)[subCellId][2*q]+=rhoTempSpinPolarized[2*q];
				(*rhoOutValuesSpinPolarized)[subCellId][2*q+1]+=rhoTempSpinPolarized[2*q+1];

				if(dftParameters::xc_id == 4)
				    for(unsigned int idim=0; idim<3; ++idim)
				    {
				      (*gradRhoOutValuesSpinPolarized)[subCellId][6*q+idim]
					  +=gradRhoTempSpinPolarized[6*q + idim];
				      (*gradRhoOutValuesSpinPolarized)[subCellId][6*q+3+idim]
				          +=gradRhoTempSpinPolarized[6*q + 3+idim];
				   }

				(*rhoOutValues)[subCellId][q]+= rhoTempSpinPolarized[2*q] + rhoTempSpinPolarized[2*q+1];

				if(dftParameters::xc_id == 4)
				  for(unsigned int idim=0; idim<3; ++idim)
				    (*gradRhoOutValues)[subCellId][3*q + idim]
					+= gradRhoTempSpinPolarized[6*q + idim]
					   + gradRhoTempSpinPolarized[6*q + 3+idim];
			 }
			 else
			 {
				(*rhoOutValues)[subCellId][q] += rhoTemp[q];

				 if(dftParameters::xc_id == 4)
				     for(unsigned int idim=0; idim<3; ++idim)
				        (*gradRhoOutValues)[subCellId][3*q+idim]+= gradRhoTemp[3*q+idim];
			 }
		    }
	      }//subcell loop
       }//macro cell loop
   }//eigenvectors block loop


   //gather density from all pools
   if (dftParameters::npool>1)
       for (unsigned int cell=0; cell<matrix_free_data.n_macro_cells(); ++cell)
       {
	      const unsigned int numSubCells=matrix_free_data.n_components_filled(cell);

	      for (unsigned int iSubCell=0; iSubCell<numSubCells; ++iSubCell)
	      {
		    const dealii::CellId subCellId=matrix_free_data.get_cell_iterator(cell,iSubCell)->id();

		    dealii::Utilities::MPI::sum((*rhoOutValues)[subCellId],
						interpoolcomm,
						(*rhoOutValues)[subCellId]);
		    if(dftParameters::xc_id == 4)
		       dealii::Utilities::MPI::sum((*gradRhoOutValues)[subCellId],
						   interpoolcomm,
						   (*gradRhoOutValues)[subCellId]);

		    if (dftParameters::spinPolarized==1)
		    {
			dealii::Utilities::MPI::sum((*rhoOutValuesSpinPolarized)[subCellId],
						     interpoolcomm,
						     (*rhoOutValuesSpinPolarized)[subCellId]);
			if(dftParameters::xc_id == 4)
			   dealii::Utilities::MPI::sum((*gradRhoOutValuesSpinPolarized)[subCellId],
						       interpoolcomm,
						       (*gradRhoOutValuesSpinPolarized)[subCellId]);
		    }
	      }
       }

  //pop out rhoInVals and rhoOutVals if their size exceeds mixing history size
  if(rhoInVals.size() == dftParameters::mixingHistory)
    {
      rhoInVals.pop_front();
      rhoOutVals.pop_front();

      if(dftParameters::spinPolarized==1)
      {
	  rhoInValsSpinPolarized.pop_front();
	  rhoOutValsSpinPolarized.pop_front();
      }

      if(dftParameters::xc_id == 4)//GGA
      {
	  gradRhoInVals.pop_front();
	  gradRhoOutVals.pop_front();
      }

      if(dftParameters::spinPolarized==1 && dftParameters::xc_id==4)
      {
	  gradRhoInValsSpinPolarized.pop_front();
	  gradRhoOutValsSpinPolarized.pop_front();
      }
    }

}


//rho data reinitilization without remeshing. The rho out of last ground state solve is made the rho in of the new solve
template<unsigned int FEOrder>
void dftClass<FEOrder>::noRemeshRhoDataInit()
{
  //create temporary copies of rho Out data
  std::map<dealii::CellId, std::vector<double> > rhoOutValuesCopy=*(rhoOutValues);

  std::map<dealii::CellId, std::vector<double> > gradRhoOutValuesCopy;
  if (dftParameters::xc_id==4)
  {
     gradRhoOutValuesCopy=*(gradRhoOutValues);
  }

  std::map<dealii::CellId, std::vector<double> > rhoOutValuesSpinPolarizedCopy;
  if(dftParameters::spinPolarized==1)
  {
     rhoOutValuesSpinPolarizedCopy=*(rhoOutValuesSpinPolarized);

  }

  std::map<dealii::CellId, std::vector<double> > gradRhoOutValuesSpinPolarizedCopy;
  if(dftParameters::spinPolarized==1 && dftParameters::xc_id==4)
  {
     gradRhoOutValuesSpinPolarizedCopy=*(gradRhoOutValuesSpinPolarized);

  }
  //cleanup of existing rho Out and rho In data
  clearRhoData();

  ///copy back temporary rho out to rho in data
  rhoInVals.push_back(rhoOutValuesCopy);
  rhoInValues=&(rhoInVals.back());

  if (dftParameters::xc_id==4)
  {
    gradRhoInVals.push_back(gradRhoOutValuesCopy);
    gradRhoInValues=&(gradRhoInVals.back());
  }

  if(dftParameters::spinPolarized==1)
  {
    rhoInValsSpinPolarized.push_back(rhoOutValuesSpinPolarizedCopy);
    rhoInValuesSpinPolarized=&(rhoInValsSpinPolarized.back());
  }

  if (dftParameters::xc_id==4 && dftParameters::spinPolarized==1)
  {
    gradRhoInValsSpinPolarized.push_back(gradRhoOutValuesSpinPolarizedCopy);
    gradRhoInValuesSpinPolarized=&(gradRhoInValsSpinPolarized.back());
  }

  normalizeRho();

}
