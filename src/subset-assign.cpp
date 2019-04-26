#include <rray.h>
#include <tools/tools.h>
#include <dispatch.h>
#include <subset-tools.h>

// Required for the assignment step
#include <xtensor/xarray.hpp>

// xtensor can't handle assigning to a requested dimension of 0
// i.e. this crashes) x[0, 1] <- 99
// this should always be a no-op so we just check for it and return x
bool any_zero_length_indexer(Rcpp::List indexer) {
  int n = indexer.size();

  for (int i = 0; i < n; ++i) {
    if (Rf_length(indexer[i]) == 0) {
      return true;
    }
  }

  return false;
}

void validate_broadcastable(Rcpp::IntegerVector x_dim, Rcpp::IntegerVector dim) {
  Rcpp::LogicalVector ok = (x_dim == dim | x_dim == 1 | dim == 0);
  if (Rcpp::is_true(Rcpp::any(!ok))) {
    Rcpp::stop("Non-broadcastable dimensions.");
  }
}

// Ensure that we can actually perform the assignment so we don't segfault
template <class E1, class E2>
void validate_broadcastable_shapes(E1 x, E2 to) {
  auto x_shape = x.shape();
  Rcpp::IntegerVector x_dim(x_shape.begin(), x_shape.end());

  auto to_shape = to.shape();
  Rcpp::IntegerVector to_dim(to_shape.begin(), to_shape.end());

  validate_broadcastable(x_dim, to_dim);
}

template <typename T>
auto rray__increase_dims_view(const xt::rarray<T>& x, const int& dims) {

  using vec_size_t = typename std::vector<std::size_t>;

  // Reshape `x` to have the dimensionality of `dims`
  Rcpp::IntegerVector x_dim = rray__dim(SEXP(x));

  Rcpp::IntegerVector x_view_dim = rray__increase_dims(x_dim, dims);
  const vec_size_t& xt_view_dim = Rcpp::as<vec_size_t>(x_view_dim);

  auto x_view = xt::reshape_view(x, xt_view_dim, xt::layout_type::column_major);

  return x_view;
}

template <typename T>
xt::rarray<T> rray__subset_assign_impl(const xt::rarray<T>& x,
                                       Rcpp::List indexer,
                                       Rcpp::RObject value_) {

  if (any_zero_length_indexer(indexer)) {
    return x;
  }

  xt::rarray<T> value(value_);

  // Reshape `xt_value` to have the dimensionality of `x`
  const int& x_dims = rray__dims(SEXP(x));
  auto value_view = rray__increase_dims_view(value, x_dims);

  // Request a copy of `x` that we can assign to
  // `x` comes in as a `const&` that we can't modify directly
  xt::rarray<T> out = x;

  if (is_stridable(indexer)) {
    xt::xstrided_slice_vector sv = build_strided_slice_vector(indexer);
    auto x_subset_view = xt::strided_view(out, sv);
    validate_broadcastable_shapes(value_view, x_subset_view);
    x_subset_view = value_view;
  }
  else {
    xt::xdynamic_slice_vector sv = build_dynamic_slice_vector(indexer);
    auto x_subset_view = xt::dynamic_view(out, sv);
    validate_broadcastable_shapes(value_view, x_subset_view);
    x_subset_view = value_view;
  }

  return out;
}

// [[Rcpp::export]]
Rcpp::RObject rray__subset_assign(Rcpp::RObject x, Rcpp::List indexer, Rcpp::RObject value) {
  DISPATCH_UNARY_TWO(rray__subset_assign_impl, x, indexer, value);
}
