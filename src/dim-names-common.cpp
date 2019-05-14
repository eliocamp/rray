#include <rray.h>
#include <dispatch.h>

// -----------------------------------------------------------------------------

// Adds meta names of dim_names onto new_dim_names, allowing for going
// up or down in dimensionality.
void reshape_meta_dim_names(Rcpp::List new_dim_names, Rcpp::List dim_names) {

  if (r_is_null(dim_names.names())) {
    return;
  }

  int n_old_meta_names = dim_names.size();
  int n_new_meta_names = new_dim_names.size();

  // Default value is `""`
  Rcpp::CharacterVector new_meta_names(n_new_meta_names);
  Rcpp::CharacterVector old_meta_names = dim_names.names();

  // Fill in the current meta names
  for (int i = 0; i < n_old_meta_names; ++i) {

    // Allow for going down in dimensionality
    if (i == n_new_meta_names) {
      break;
    }

    new_meta_names[i] = old_meta_names[i];
  }

  new_dim_names.names() = new_meta_names;
}

// rray__reshape_dim_names() takes `dim_names` and grows or shrinks it based
// on `dim`. Specifically it can do:
// - Grow the dimensionaltiy by appending `NULL` elements
// - Shrink the dimensionality by removing elements from the back
// - Keep dimensionality, but set names to `NULL` if the new `dim` for that
//   axis is not the same length as the original names (i.e. there were
//   5 row names, but the new row length is 3)
// - Grow meta names dimensionality by adding them as `""`
// - Shrink meta names dimensionality

// [[Rcpp::export(rng = false)]]
Rcpp::List rray__reshape_dim_names(Rcpp::List dim_names,
                                   Rcpp::IntegerVector dim) {

  int n_old_dim_names = dim_names.size();
  int n_new_dim_names = dim.size();

  Rcpp::List new_dim_names = rray__new_empty_dim_names(n_new_dim_names);

  reshape_meta_dim_names(new_dim_names, dim_names);

  for (int i = 0; i < n_new_dim_names; ++i) {

    // If reshaping down in dimensionality
    // and we hit the required number, we are done
    if (i == n_old_dim_names) {
      break;
    }

    // Nothing to restore
    if (r_is_null(dim_names[i])) {
      continue;
    }

    Rcpp::CharacterVector axis_names = dim_names[i];

    // To be restorable, the size of the old names
    // must match the new dim
    if (axis_names.size() != dim[i]) {
      continue;
    }

    new_dim_names[i] = axis_names;
  }

  return new_dim_names;
}

// -----------------------------------------------------------------------------

// There is an assumption that the sizes are the same on the way in
void validate_equal_dim_name_sizes(Rcpp::List x_dim_names, Rcpp::List y_dim_names) {
  if (x_dim_names.size() != y_dim_names.size()) {
    Rcpp::stop(
      "`x_dim_names` size (%i) must match `y_dim_names` size (%i)",
      x_dim_names.size(),
      y_dim_names.size()
    );
  }
}

Rcpp::RObject coalesce_axis_names(Rcpp::RObject x_axis_names,
                                  Rcpp::RObject y_axis_names) {

  int n_x_axis_names = Rf_xlength(x_axis_names);
  int n_y_axis_names = Rf_xlength(y_axis_names);

  if (n_x_axis_names == n_y_axis_names) {
    return x_axis_names;
  }
  else if (n_x_axis_names == 0) {
    return y_axis_names;
  }
  else if (n_y_axis_names == 0) {
    return x_axis_names;
  }

  Rcpp::stop("Incompatible dim_name lengths.");
}

// [[Rcpp::export(rng = false)]]
Rcpp::RObject rray__coalesce_meta_dim_names(Rcpp::RObject x_meta_dim_names,
                                            Rcpp::RObject y_meta_dim_names) {

  if (r_is_null(x_meta_dim_names)) {
    return y_meta_dim_names;
  }

  return x_meta_dim_names;
}

// [[Rcpp::export(rng = false)]]
Rcpp::List rray__coalesce_dim_names(Rcpp::List x_dim_names,
                                    Rcpp::List y_dim_names) {

  validate_equal_dim_name_sizes(x_dim_names, y_dim_names);

  int n = x_dim_names.size();
  Rcpp::List new_dim_names(n);

  for (int i = 0; i < n; ++i) {
    new_dim_names[i] = coalesce_axis_names(x_dim_names[i], y_dim_names[i]);
  }

  new_dim_names.names() = rray__coalesce_meta_dim_names(
    x_dim_names.names(),
    y_dim_names.names()
  );

  return new_dim_names;
}

// -----------------------------------------------------------------------------

// [[Rcpp::export(rng = false)]]
Rcpp::List rray__dim_names2(Rcpp::RObject x, Rcpp::RObject y) {

  Rcpp::IntegerVector dim = rray__dim2(rray__dim(x), rray__dim(y));

  Rcpp::List reshaped_x_dim_names = rray__reshape_dim_names(rray__dim_names(x), dim);
  Rcpp::List reshaped_y_dim_names = rray__reshape_dim_names(rray__dim_names(y), dim);

  Rcpp::List common_dim_names = rray__coalesce_dim_names(
    reshaped_x_dim_names,
    reshaped_y_dim_names
  );

  return common_dim_names;
}
