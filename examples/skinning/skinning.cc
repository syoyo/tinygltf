
#if 0

jointMatrix(j) = 
    globalTransform^-1 *      // The "^-1" here means the inverse of this transform
    globalJointTransform *
    inverseBindMatrix(j);

#endif
