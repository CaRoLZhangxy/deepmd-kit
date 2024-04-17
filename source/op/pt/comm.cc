// SPDX-License-Identifier: LGPL-3.0-or-later
#ifdef GOOGLE_CUDA
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif
#include <torch/torch.h>
#ifdef USE_MPI
#include <mpi.h>
template <typename T>
static MPI_Datatype get_mpi_type();

template <>
MPI_Datatype get_mpi_type<float>() {
  return MPI_FLOAT;
}

template <>
MPI_Datatype get_mpi_type<double>() {
  return MPI_DOUBLE;
}
#endif
class Border : public torch::autograd::Function<Border> {
 public:
  static torch::autograd::variable_list forward(
      torch::autograd::AutogradContext* ctx,
      const torch::Tensor& sendlist_tensor,
      const torch::Tensor& sendproc_tensor,
      const torch::Tensor& recvproc_tensor,
      const torch::Tensor& sendnum_tensor,
      const torch::Tensor& recvnum_tensor,
      const torch::Tensor& g1,
      const torch::Tensor& communicator_tensor,
      const torch::Tensor& nlocal_tensor,
      const torch::Tensor& nghost_tensor) {
    bool type_flag = (g1.dtype() == torch::kDouble) ? true : false;
    using FPTYPE = float;
    if (type_flag) {
      using FPTYPE = double;
    }

    ctx->save_for_backward({sendlist_tensor, sendproc_tensor, recvproc_tensor,
                            sendnum_tensor, recvnum_tensor, communicator_tensor,
                            nlocal_tensor, nghost_tensor});
    int** sendlist = reinterpret_cast<int**>(sendlist_tensor.data_ptr());
    int* sendproc = sendproc_tensor.data_ptr<int>();
    int* recvproc = recvproc_tensor.data_ptr<int>();
    int* sendnum = sendnum_tensor.data_ptr<int>();
    int* recvnum = recvnum_tensor.data_ptr<int>();
    int tensor_size = g1.size(1);
    int nswap = sendproc_tensor.size(0);

    int nlocal = nlocal_tensor.item<int>();
    int nghost = nghost_tensor.item<int>();
    int ntotal = nlocal + nghost;
    torch::Tensor recv_g1_tensor = g1;

    FPTYPE* recv_g1 = recv_g1_tensor.data_ptr<FPTYPE>() + nlocal * tensor_size;

#ifdef USE_MPI
    int me;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm world;
    unpack_communicator(communicator_tensor, world);
    MPI_Datatype mpi_type = get_mpi_type<FPTYPE>();
    MPI_Request request;
#endif
    auto int32_options = torch::TensorOptions().dtype(torch::kInt32);

    for (int iswap = 0; iswap < nswap; ++iswap) {
      int nrecv = recvnum[iswap];
      int nsend = sendnum[iswap];
      torch::Tensor isendlist =
          torch::from_blob(sendlist[iswap], {nsend}, int32_options)
              .to(recv_g1_tensor.device());
      torch::Tensor send_g1_tensor = recv_g1_tensor.index_select(0, isendlist);
      FPTYPE* send_g1 = send_g1_tensor.data_ptr<FPTYPE>();
#ifdef USE_MPI
      if (sendproc[iswap] != me) {
        if (nrecv) {
          MPI_Irecv(recv_g1, nrecv * tensor_size, mpi_type, recvproc[iswap], 0,
                    world, &request);
        }
        if (nsend) {
          MPI_Send(send_g1, nsend * tensor_size, mpi_type, sendproc[iswap], 0,
                   world);
        }
        if (nrecv) {
          MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
      } else {
#endif
#ifdef GOOGLE_CUDA
        cudaMemcpy(recv_g1, send_g1, nsend * tensor_size * sizeof(FPTYPE),
                   cudaMemcpyDeviceToDevice);
#else
      memcpy(recv_g1, send_g1, nsend * tensor_size * sizeof(FPTYPE));
#endif
#ifdef USE_MPI
      }
#endif
      recv_g1 += nrecv * tensor_size;
    }

    return {recv_g1_tensor};
  }
  static torch::autograd::variable_list backward(
      torch::autograd::AutogradContext* ctx,
      torch::autograd::variable_list grad_output) {
#ifdef GOOGLE_CUDA
    cudaDeviceSynchronize();
#endif

    torch::autograd::variable_list saved_variables = ctx->get_saved_variables();
    torch::Tensor sendlist_tensor = saved_variables[0];
    torch::Tensor sendproc_tensor = saved_variables[1];
    torch::Tensor recvproc_tensor = saved_variables[2];
    torch::Tensor sendnum_tensor = saved_variables[3];
    torch::Tensor recvnum_tensor = saved_variables[4];
    torch::Tensor communicator_tensor = saved_variables[5];
    torch::Tensor nlocal_tensor = saved_variables[6];
    torch::Tensor nghost_tensor = saved_variables[7];

    torch::Tensor d_local_g1_tensor = grad_output[0];
    bool type_flag =
        (d_local_g1_tensor.dtype() == torch::kDouble) ? true : false;
    using FPTYPE = float;
    if (type_flag) {
      using FPTYPE = double;
    }

    int** recvlist = reinterpret_cast<int**>(sendlist_tensor.data_ptr());
    // swap send and recv here
    int* recvproc = sendproc_tensor.data_ptr<int>();
    int* sendproc = recvproc_tensor.data_ptr<int>();
    int* recvnum = sendnum_tensor.data_ptr<int>();
    int* sendnum = recvnum_tensor.data_ptr<int>();

    FPTYPE* local_g1 = d_local_g1_tensor.data_ptr<FPTYPE>();
    int tensor_size = d_local_g1_tensor.size(1);
    int nswap = sendproc_tensor.size(0);

    int nlocal = nlocal_tensor.item<int>();
    int nghost = nghost_tensor.item<int>();
    int ntotal = nlocal + nghost;

    torch::Tensor send_g1_tensor = d_local_g1_tensor;

    int max_recvnum = sendnum_tensor.max().item<int>();
    auto options = torch::TensorOptions()
                       .dtype(torch::kFloat64)
                       .device(d_local_g1_tensor.device());
    torch::Tensor recv_g1_tensor =
        torch::empty({max_recvnum, tensor_size}, options);
    FPTYPE* recv_g1 = recv_g1_tensor.data_ptr<FPTYPE>();
    FPTYPE* send_g1 = send_g1_tensor.data_ptr<FPTYPE>() + ntotal * tensor_size;
#ifdef USE_MPI
    MPI_Comm world;
    unpack_communicator(communicator_tensor, world);
    int me;
    MPI_Comm_rank(world, &me);
    MPI_Datatype mpi_type = get_mpi_type<FPTYPE>();
    MPI_Request request;
#endif
    std::string msg;

    int end = ntotal;
    auto int32_options = torch::TensorOptions().dtype(torch::kInt32);
    for (int iswap = nswap - 1; iswap >= 0; --iswap) {
      int nrecv = recvnum[iswap];
      int nsend = sendnum[iswap];

      torch::Tensor irecvlist;
      if (nrecv) {
        irecvlist = torch::from_blob(recvlist[iswap], {nrecv}, int32_options)
                        .to(d_local_g1_tensor.device());
      }
      if (nsend) {
        send_g1 -= nsend * tensor_size;
      }
#ifdef USE_MPI
      if (sendproc[iswap] != me) {
        if (nrecv) {
          MPI_Irecv(recv_g1, nrecv * tensor_size, mpi_type, recvproc[iswap], 0,
                    world, &request);
        }
        if (nsend) {
          MPI_Send(send_g1, nsend * tensor_size, mpi_type, sendproc[iswap], 0,
                   world);
        }
        if (nrecv) {
          MPI_Wait(&request, MPI_STATUS_IGNORE);
        }
      } else {
#endif
        if (nrecv) {
#ifdef GOOGLE_CUDA
          cudaMemcpy(recv_g1, send_g1, nrecv * tensor_size * sizeof(FPTYPE),
                     cudaMemcpyDeviceToDevice);
#else
        memcpy(recv_g1, send_g1, nrecv * tensor_size * sizeof(FPTYPE));
#endif
        }
#ifdef USE_MPI
      }
#endif
      if (nrecv) {
        d_local_g1_tensor.index_add_(0, irecvlist,
                                     recv_g1_tensor.slice(0, 0, nrecv));
      }
    }
#ifdef GOOGLE_CUDA
    cudaDeviceSynchronize();
#endif

    return {torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor(), torch::Tensor(), d_local_g1_tensor,
            torch::Tensor(), torch::Tensor(), torch::Tensor(),
            torch::Tensor()};
  }
#ifdef USE_MPI
  static void unpack_communicator(const torch::Tensor& communicator_tensor,
                                  MPI_Comm& mpi_comm) {
    long int* communicator = communicator_tensor.data_ptr<long int>();
    int* int_ptr = reinterpret_cast<int*>(communicator);//in order to solve mpich type, may cause error
    mpi_comm = reinterpret_cast<MPI_Comm>(*int_ptr);
  }
#endif
};
std::vector<torch::Tensor> border_op(const torch::Tensor& sendlist_tensor,
                                     const torch::Tensor& sendproc_tensor,
                                     const torch::Tensor& recvproc_tensor,
                                     const torch::Tensor& sendnum_tensor,
                                     const torch::Tensor& recvnum_tensor,
                                     const torch::Tensor& g1_tensor,
                                     const torch::Tensor& communicator_tensor,
                                     const torch::Tensor& nlocal_tensor,
                                     const torch::Tensor& nghost_tensor) {
  return Border::apply(sendlist_tensor, sendproc_tensor, recvproc_tensor,
                       sendnum_tensor, recvnum_tensor, g1_tensor,
                       communicator_tensor, nlocal_tensor, nghost_tensor);
}

TORCH_LIBRARY_FRAGMENT(deepmd, m) { m.def("border_op", border_op); }