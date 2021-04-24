#ifndef PREDICTOR_OPERATIONS_HEADER
#define PREDICTOR_OPERATIONS_HEADER

uint8_t midpoint(uint8_t a, uint8_t b){
	return a + (b - a) / 2;
}

uint16_t midpoint(uint16_t a, uint16_t b){
	return a + (b - a) / 2;
}

uint8_t median(uint8_t a, uint8_t b, uint8_t c){
	if(a > b){
		if(b > c){
			return b;
		}
		else if(c > a){
			return a;
		}
		else{
			return c;
		}
	}
	else{
		if(b < c){
			return b;
		}
		else if(c > a){
			return c;
		}
		else{
			return a;
		}
	}
}

uint16_t median(uint16_t a, uint16_t b, uint16_t c){
	if(a > b){
		if(b > c){
			return b;
		}
		else if(c > a){
			return a;
		}
		else{
			return c;
		}
	}
	else{
		if(b < c){
			return b;
		}
		else if(c > a){
			return c;
		}
		else{
			return a;
		}
	}
}

uint8_t average3(uint8_t a, uint8_t b, uint8_t c){
	return (uint8_t)(((int)a + (int)b + (int)c)/3);
}

uint16_t average3(uint16_t a, uint16_t b, uint16_t c){
	return (uint16_t)(((int)a + (int)b + (int)c)/3);
}

uint8_t paeth(uint8_t A,uint8_t B,uint8_t C){
	int p = A + B - C;
	int Ap = std::abs(A - p);
	int Bp = std::abs(B - p);
	int Cp = std::abs(C - p);
	if(Ap < Bp){
		if(Ap < Cp){
			return (uint8_t)A;
		}
		return (uint8_t)C;
	}
	else{
		if(Bp < Cp){
			return (uint8_t)B;
		}
		return (uint8_t)C;
	}
}

uint16_t paeth(uint16_t A,uint16_t B,uint16_t C){
	int p = A + B - C;
	int Ap = std::abs(A - p);
	int Bp = std::abs(B - p);
	int Cp = std::abs(C - p);
	if(Ap < Bp){
		if(Ap < Cp){
			return (uint16_t)A;
		}
		return (uint16_t)C;
	}
	else{
		if(Bp < Cp){
			return (uint16_t)B;
		}
		return (uint16_t)C;
	}
}

#endif //PREDICTOR_OPERATIONS_HEADER
